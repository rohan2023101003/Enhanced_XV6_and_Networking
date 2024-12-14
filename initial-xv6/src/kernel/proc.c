#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

struct cpu cpus[NCPU];

struct proc proc[NPROC];

struct proc *initproc;
#ifdef MLFQ
int boost_ticks=0;
struct proc* queues[NUM_QUEUES][MAX_QUEUE];
int time_slice[NUM_QUEUES]={1,4,8,16};
int queue_lengths[NUM_QUEUES];
void init_q(){
  for(int i=0;i<NUM_QUEUES;i++){
    queue_lengths[i]=0;
    switch(i){
      case 0 :
        time_slice[0]=1;
        break;
      case 1:
        time_slice[1]=4;
        break;
      case 2:
        time_slice[2]=8;
        break;
      case 3:
        time_slice[3]=16;
        break;
      default: break;
    }
    for(int j=0;j<MAX_QUEUE;j++){
      queues[i][j]=0;
    }
    queue_lengths[i]=0;
  }
}
void queue_at_end(struct proc* p,int queue_no){
  if(queue_lengths[queue_no] >= MAX_QUEUE){
    printf("Queue %d is already full!!\n",queue_no);
    exit(1);
  }
  queues[queue_no][queue_lengths[queue_no]]=p;
  queue_lengths[queue_no]++;
  p->queue_number = queue_no;
  p->position_in_queue = queue_lengths[queue_no]-1;
  p->in_queue=1;
}
void queue_at_start(struct proc* p ,int queue_no){
  if(queue_lengths[queue_no] >= MAX_QUEUE){
    printf("Queue %d is already full!!\n",queue_no);
    exit(1);
  }
  for(int i = queue_lengths[queue_no]-1;i>=0;i--){
    queues[queue_no][i+1]=queues[queue_no][i];
    queues[queue_no][i+1]->position_in_queue++;
  }
  queues[queue_no][0]=p;
  queue_lengths[queue_no]++;
  p->in_queue=1;
  p->position_in_queue=0;
  p->queue_number=queue_no;
}
struct proc* dequeue_from_start(int queue_no){
  if(queue_lengths[queue_no]<=0){
    printf("queue %d is empty!!\n",queue_no);
    exit(1);
  }
  struct proc* p = queues[queue_no][0];
  queues[queue_no][0]=0;
  for(int i=0;i< queue_lengths[queue_no]-1;i++){
    queues[queue_no][i]=queues[queue_no][i+1];
  }
  queue_lengths[queue_no]--;
  p->in_queue=0;
  return p;
}
void dequeue_at_a_position(struct proc* p,int queue_no ){
  // int dq=0;
  for(int i=0;i<queue_lengths[queue_no];i++){
    if(queues[queue_no][i] < p){
      continue;
    }
    else{
      // dq=1;
      int j;
      for(j=i+1;j<queue_lengths[queue_no]-1;j++){
        queues[queue_no][j-1]=queues[queue_no][j];
      }
      queues[queue_no][queue_lengths[queue_no]-1]=0;
      //  queue_lengths[queue_no]--;
      break;
    }
  }
  // if(dq){
    // printf("process found");
    if(queue_lengths[queue_no]>0)
    queue_lengths[queue_no]--;
  // }
}
#endif

//--------------------------------------
int nextpid = 1;
struct spinlock pid_lock;

extern void forkret(void);
static void freeproc(struct proc *p);

extern char trampoline[]; // trampoline.S

int max(int a, int b) {
  if (a > b)
    return a;
  return b;
}

int min(int a, int b) {
  if (a < b)
    return a;
  return b;
}

static uint64 state0 = 1234567890123456789ULL;
static uint64 state1 = 9876543210987654321ULL;

uint64 xorshift128plus(void) {
  uint64 x = state0;
  uint64 const y = state1;
  state0 = y;
  x ^= x << 23;                           // a
  state1 = x ^ y ^ (x >> 17) ^ (y >> 26); // b, c
  return state1 + y;
}

uint random(void) { return (uint)(xorshift128plus() >> 32); }

int randomrange(int lo, int hi) {
  uint range = (uint)(hi - lo + 1);
  return (int)(random() % range) + lo;
}

// Function to seed the random number generator
void seed_random(uint64 seed1, uint64 seed2) {
  state0 = seed1;
  state1 = seed2;
}
// helps ensure that wakeups of wait()ing
// parents are not lost. helps obey the
// memory model when using p->parent.
// must be acquired before any p->lock.
struct spinlock wait_lock;

// Allocate a page for each process's kernel stack.
// Map it high in memory, followed by an invalid
// guard page.
void proc_mapstacks(pagetable_t kpgtbl) {
  struct proc *p;

  for (p = proc; p < &proc[NPROC]; p++) {
    char *pa = kalloc();
    if (pa == 0)
      panic("kalloc");
    uint64 va = KSTACK((int)(p - proc));
    kvmmap(kpgtbl, va, (uint64)pa, PGSIZE, PTE_R | PTE_W);
  }
}

// initialize the proc table.
void procinit(void) {
  struct proc *p;

  initlock(&pid_lock, "nextpid");
  initlock(&wait_lock, "wait_lock");
  for (p = proc; p < &proc[NPROC]; p++) {
    initlock(&p->lock, "proc");
    p->state = UNUSED;
    p->kstack = KSTACK((int)(p - proc));
  }
}
// Must be called with interrupts disabled,
// to prevent race with process being moved
// to a different CPU.
int cpuid() {
  int id = r_tp();
  return id;
}

// Return this CPU's cpu struct.
// Interrupts must be disabled.
struct cpu *mycpu(void) {
  int id = cpuid();
  struct cpu *c = &cpus[id];
  return c;
}

// Return the current struct proc *, or zero if none.
struct proc *myproc(void) {
  push_off();
  struct cpu *c = mycpu();
  struct proc *p = c->proc;
  pop_off();
  return p;
}
uint64 priority(struct proc *p) {
  p->good = 5;
  if (p->time_ran + p->time_slept)
    p->good = (p->time_slept * 10 / (p->time_ran + p->time_slept));
  uint64 DP = max(0, min(p->priority - p->good + 5, 100));
  return DP;
}
int allocpid() {
  int pid;

  acquire(&pid_lock);
  pid = nextpid;
  nextpid = nextpid + 1;
  release(&pid_lock);

  return pid;
}

// Look in the process table for an UNUSED proc.
// If found, initialize state required to run in the kernel,
// and return with p->lock held.
// If there are no free procs, or a memory allocation fails, return 0.

static struct proc *allocproc(void) {
  struct proc *p;

  for (p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);
    if (p->state == UNUSED) {
      goto found;
    } else {
      release(&p->lock);
    }
  }
  return 0;

found:
  p->pid = allocpid();
  // added  new
  //  ----------
  memset(p->syscall_count, 0, sizeof(p->syscall_count));
  // -----------
  p->state = USED;

  // Allocate a trapframe page.
  p->time_ran = 0;
  p->time_slept = 0;
  p->time_ends = 0;
  p->tickets = 1;          // Default to 1 ticket for LBS
  p->arrival_time = ticks; // Set arrival time
                           // Initialize the MLFQ-related fields
  // p->queue_number = 0;           // All new processes start at queue 0
  // p->ticks_in_current_queue = 0; // Initialize ticks to 0
  // p->wait_time = 0;              // Initialize wait time to 0
  // p->in_queue = 0;               // Initially, the process is not in a queue
  // p->position_in_queue = -1;     // Initialize position to -1 (not in queue)
  //-------------------------

  if ((p->trapframe = (struct trapframe *)kalloc()) == 0) {
    freeproc(p);
    release(&p->lock);
    return 0;
  }
  if ((p->copy_of_tapeframe = (struct trapframe *)kalloc()) == 0) {
    release(&p->lock);
    return 0;
  }
  p->good = 5;
  p->time_waited = 0;
  // An empty user page table.
  p->pagetable = proc_pagetable(p);
  if (p->pagetable == 0) {
    freeproc(p);
    release(&p->lock);
    return 0;
  }
  // Set up new context to start executing at forkret,
  // which returns to user space.
  memset(&p->context, 0, sizeof(p->context));
  p->context.ra = (uint64)forkret;
  p->context.sp = p->kstack + PGSIZE;
  #ifdef MLFQ
  p->queue_number = 0;
  p->wait_time = 0;
  p->in_queue = 0;
#endif
  return p;
}
int set_priority(int new_priority, int proc_pid) {
  int old_priority = 0, found = 0;
  struct proc *p;
  for (p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);
    if (p->pid == proc_pid) {
      found = 1;
      old_priority = p->priority;
      p->time_ran = 0;
      p->priority = new_priority;
      release(&p->lock);
      if (old_priority >= new_priority)
        break;
      yield();
      break;
    }
    release(&p->lock);
  }
  if (!found)
    printf("no process with pid : %d exists\n", proc_pid);
  return old_priority;
}
// free a proc structure and the data hanging from it,
// including user pages.
// p->lock must be held.
static void freeproc(struct proc *p) {
  if (p->trapframe)
    kfree((void *)p->trapframe);
  if (p->copy_of_tapeframe)
    kfree((void *)p->copy_of_tapeframe);
  p->trapframe = 0;
  if (p->pagetable)
    proc_freepagetable(p->pagetable, p->sz);
  p->pagetable = 0;
  p->sz = 0;
  // p->pid = 0; //comment  if needed

  p->parent = 0;
  p->name[0] = 0;
  p->chan = 0;
  p->killed = 0;
  p->xstate = 0;
  p->state = UNUSED;
  #ifdef MLFQ
  p->in_queue = 0;
#endif
}

// Create a user page table for a given process, with no user memory,
// but with trampoline and trapframe pages.
pagetable_t proc_pagetable(struct proc *p) {
  pagetable_t pagetable;

  // An empty page table.
  pagetable = uvmcreate();
  if (pagetable == 0)
    return 0;

  // map the trampoline code (for system call return)
  // at the highest user virtual address.
  // only the supervisor uses it, on the way
  // to/from user space, so not PTE_U.
  if (mappages(pagetable, TRAMPOLINE, PGSIZE, (uint64)trampoline,
               PTE_R | PTE_X) < 0) {
    uvmfree(pagetable, 0);
    return 0;
  }

  // map the trapframe page just below the trampoline page, for
  // trampoline.S.
  if (mappages(pagetable, TRAPFRAME, PGSIZE, (uint64)(p->trapframe),
               PTE_R | PTE_W) < 0) {
    uvmunmap(pagetable, TRAMPOLINE, 1, 0);
    uvmfree(pagetable, 0);
    return 0;
  }

  return pagetable;
}

// Free a process's page table, and free the
// physical memory it refers to.
void proc_freepagetable(pagetable_t pagetable, uint64 sz) {
  uvmunmap(pagetable, TRAMPOLINE, 1, 0);
  uvmunmap(pagetable, TRAPFRAME, 1, 0);
  uvmfree(pagetable, sz);
}

// a user program that calls exec("/init")
// assembled from ../user/initcode.S
// od -t xC ../user/initcode
uchar initcode[] = {0x17, 0x05, 0x00, 0x00, 0x13, 0x05, 0x45, 0x02, 0x97,
                    0x05, 0x00, 0x00, 0x93, 0x85, 0x35, 0x02, 0x93, 0x08,
                    0x70, 0x00, 0x73, 0x00, 0x00, 0x00, 0x93, 0x08, 0x20,
                    0x00, 0x73, 0x00, 0x00, 0x00, 0xef, 0xf0, 0x9f, 0xff,
                    0x2f, 0x69, 0x6e, 0x69, 0x74, 0x00, 0x00, 0x24, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// Set up first user process.
void userinit(void) {
  struct proc *p;

  p = allocproc();
  initproc = p;
  #ifdef MLFQ
  init_q();
  #endif
  // allocate one user page and copy initcode's instructions
  // and data into it.
  uvmfirst(p->pagetable, initcode, sizeof(initcode));
  p->sz = PGSIZE;

  // prepare for the very first "return" from kernel to user.
  p->trapframe->epc = 0;     // user program counter
  p->trapframe->sp = PGSIZE; // user stack pointer

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  p->state = RUNNABLE;
  release(&p->lock);
}

// Grow or shrink user memory by n bytes.
// Return 0 on success, -1 on failure.
int growproc(int n) {
  uint64 sz;
  struct proc *p = myproc();

  sz = p->sz;
  if (n > 0) {
    if ((sz = uvmalloc(p->pagetable, sz, sz + n, PTE_W)) == 0) {
      return -1;
    }
  } else if (n < 0) {
    sz = uvmdealloc(p->pagetable, sz, sz + n);
  }
  p->sz = sz;
  return 0;
}

// Create a new process, copying the parent.
// Sets up child kernel stack to return as if from fork() system call.
int fork(void) {
  int i, pid;
  struct proc *np;
  struct proc *p = myproc();
  // np->tickets = p->tickets;
  // Allocate process.
  if ((np = allocproc()) == 0) {
    return -1;
  }

  // Copy user memory from parent to child.
  if (uvmcopy(p->pagetable, np->pagetable, p->sz) < 0) {
    freeproc(np);
    release(&np->lock);
    return -1;
  }
  np->sz = p->sz;

  // copy saved user registers.
  *(np->trapframe) = *(p->trapframe);

  // Cause fork to return 0 in the child.
  np->trapframe->a0 = 0;
  np->tickets = p->tickets;
  // increment reference counts on open file descriptors.
  for (i = 0; i < NOFILE; i++)
    if (p->ofile[i])
      np->ofile[i] = filedup(p->ofile[i]);
  np->cwd = idup(p->cwd);

  safestrcpy(np->name, p->name, sizeof(p->name));

  pid = np->pid;

  release(&np->lock);

  acquire(&wait_lock);
  np->parent = p;
  release(&wait_lock);

  acquire(&np->lock);
  np->state = RUNNABLE;
  release(&np->lock);

  return pid;
}

// Pass p's abandoned children to init.
// Caller must hold wait_lock.
void reparent(struct proc *p) {
  struct proc *pp;

  for (pp = proc; pp < &proc[NPROC]; pp++) {
    if (pp->parent == p) {
      pp->parent = initproc;
      wakeup(initproc);
    }
  }
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait().
void exit(int status) {
  struct proc *p = myproc();

  if (p == initproc)
    panic("init exiting");

  // Close all open files.
  for (int fd = 0; fd < NOFILE; fd++) {
    if (p->ofile[fd]) {
      struct file *f = p->ofile[fd];
      fileclose(f);
      p->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(p->cwd);
  end_op();
  p->cwd = 0;

  acquire(&wait_lock);

  // Give any children to init.
  reparent(p);

  // Parent might be sleeping in wait().
  wakeup(p->parent);

  acquire(&p->lock);

  p->xstate = status;
  p->state = ZOMBIE;
  p->time_ends = ticks;

  release(&wait_lock);

  // Jump into the scheduler, never to return.
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int wait(uint64 addr) {
  struct proc *pp;
  int havekids, pid;
  struct proc *p = myproc();

  acquire(&wait_lock);

  for (;;) {
    // Scan through table looking for exited children.
    havekids = 0;
    for (pp = proc; pp < &proc[NPROC]; pp++) {
      if (pp->parent == p) {
        // make sure the child isn't still in exit() or swtch().
        acquire(&pp->lock);

        havekids = 1;
        if (pp->state == ZOMBIE) {
          // Found one.
          pid = pp->pid;
          if (addr != 0 && copyout(p->pagetable, addr, (char *)&pp->xstate,
                                   sizeof(pp->xstate)) < 0) {
            release(&pp->lock);
            release(&wait_lock);
            return -1;
          }
          freeproc(pp);
          release(&pp->lock);
          release(&wait_lock);
          return pid;
        }
        release(&pp->lock);
      }
    }

    // No point waiting if we don't have any children.
    if (!havekids || killed(p)) {
      release(&wait_lock);
      return -1;
    }

    // Wait for a child to exit.
    sleep(p, &wait_lock); // DOC: wait-sleep
  }
}

// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run.
//  - swtch to start running that process.
//  - eventually that process transfers control
//    via swtch back to the scheduler.
void scheduler(void) {
  struct proc *p;
  struct cpu *c = mycpu();

  c->proc = 0;
  for (;;) {
    // Avoid deadlock by ensuring that devices can interrupt.
    intr_on();
#ifdef LBS
    uint64 total_tickets = 0;
    struct proc *candidates[NPROC];
    int num_candidates = 0;

    // First pass: count total tickets and identify runnable processes
    for (p = proc; p < &proc[NPROC]; p++) {
      acquire(&p->lock);
      if (p->state == RUNNABLE) {
        total_tickets += p->tickets;
        candidates[num_candidates++] = p;
      }
      release(&p->lock);
    }

    if (total_tickets > 0) {
      uint64 winning_ticket = randomrange(1, total_tickets);
      uint64 ticket_count = 0;
      struct proc *winner = 0;
      // printf("winning ticket: %d\n", winning_ticket);
      // Second pass: find the winning process
      for (int i = 0; i < num_candidates; i++) {
        struct proc *p = candidates[i];
        ticket_count += p->tickets;
        if (ticket_count >= winning_ticket) {
          winner = p;
          break;
        }
      }

      // Handle tie-breaking based on arrival time
      if (winner) {
        for (int i = 0; i < num_candidates; i++) {
          struct proc *p = candidates[i];
          if (p->tickets == winner->tickets &&
              p->arrival_time < winner->arrival_time) {

            winner = p;
          }
        }
      }
      // In a lottery-based scheduling system, the number of tickets that a
      // process holds represents its chance of being selected to run. When you
      // select a winning ticket, you are choosing a process to run, but the
      // number of tickets a process holds should remain constant unless you
      // specifically want to change its probability of being selected in future
      // lotteries.

      // Run the winning process
      if (winner) {
        acquire(&winner->lock);
        if (winner->state == RUNNABLE) {
          winner->state = RUNNING;
          c->proc = winner;
          swtch(&c->context, &winner->context);
          c->proc = 0;
        }
        release(&winner->lock);
      }
    }
#else
#ifdef RR
    // struct proc *p;
    for (p = proc; p < &proc[NPROC]; p++) {
      acquire(&p->lock);
      if (p->state == RUNNABLE) {
        // Switch to chosen process.  It is the process's job
        // to release its lock and then reacquire it
        // before jumping back to us.
        p->state = RUNNING;
        c->proc = p;
        swtch(&c->context, &p->context);
        // Process is done running for now.
        // It should have changed its p->state before coming back.
        c->proc = 0;
      }
      release(&p->lock);
    }
#else
#ifdef MLFQ
    //   struct proc *temp = 0;
    // // Priority Boost Logic: Every 48 ticks, move all processes to the topmost queue (priority 0)
    // if (boost_ticks >= 48) {
    //   // printf("Priority boost\n");
    //   boost_ticks = 0;  // Reset the boost tick counter
    //   for (p = proc; p < &proc[NPROC]; p++) {
    //     if (p->state == RUNNABLE && p->queue_number > 0 ) {
    //       dequeue_at_a_position(p, p->queue_number);  // Remove process from current queue
    //       p->queue_number = 0;  // Move to the highest priority queue
    //       queue_at_start(p, 0);   // Add the process to the top queue
    //       p->in_queue = 1;
    //       p->wait_time = 0;     // Reset wait time after priority boost
    //     }
    //   }
    // }

    //   // Iterate through all processes to check if they are RUNNABLE and not already in a queue.
    //   for (p = proc; p < &proc[NPROC]; p++) {
    //     if (p->state == RUNNABLE && p->in_queue == 0) {
    //       // If the process is RUNNABLE and not in a queue, push it to the end of its current priority queue.
    //       int queue_no = p->queue_number;
    //       queue_at_end(p, queue_no);   // Insert the process at the end of the corresponding queue.
    //       p->in_queue = 1;             // Mark the process as now being in the queue.
    //     }
    //   }

    //   int found = 0;  // Flag to check if a runnable process was found in the queues.

    //   // Iterate through all priority queues to find a RUNNABLE process.
    //   for (int i = 0; i < NUM_QUEUES; i++) {
    //     // While there are processes in the current queue
    //     while (queue_lengths[i] > 0) {
    //       // Dequeue the process from the start of the current priority queue.
    //       temp = dequeue_from_start(i);
    //       temp->in_queue = 0;  // Mark the process as not in the queue temporarily.

    //       // If the dequeued process is RUNNABLE, push it to the front of its queue.
    //       if (temp->state == RUNNABLE) {
    //         queue_at_start(temp, temp->queue_number);  // Reinsert the process at the front of its queue.
    //         found = 1;  // Set the flag indicating a RUNNABLE process was found.
    //         break;      // Exit the inner loop as we have found a process to run.
    //       }
    //     }

    //     // If a runnable process was found, exit the outer loop.
    //     if (found) {
    //       break;
    //     }
    //   }

    //   // If a RUNNABLE process was found and the process is valid (not NULL)
    //   if (found && temp != 0) {
    //     if (temp->state == RUNNABLE) {
    //       // Lock the process before context switching to prevent race conditions.
    //       acquire(&temp->lock);

    //       // Set the process state to RUNNING and perform the context switch.
    //       temp->state = RUNNING;
    //       c->proc = temp;               // Set the CPU's current process to the found process.
    //       swtch(&c->context, &temp->context);  // Switch the context to the found process.
    //       c->proc = 0;                  // Clear the CPU's current process after switching.

    //       // Release the process lock after the context switch is complete.
    //       release(&temp->lock);
    //     }
    //   }

    // acquire(&p->lock);
    // #ifdef MLFQ
              //      for (p = proc; p < &proc[NPROC]; p++) {
              //   printf("Time %d: Process %d priority %d\n" , ticks , p->pid,p->queue_number);
              // }
    // Priority boost every 48 ticks
    if (boost_ticks >=48) {
      boost_ticks=0;
        struct proc* b = 0;
        for (int q = 1; q < NUM_QUEUES; q++) {
            while (queue_lengths[q] > 0) {
                b = dequeue_from_start(q);
                queue_at_end(b, 0);  // Move process to topmost queue
                b->queue_number = 0;
            }
        }
    }

    // Enqueue all runnable processes
    for (p = proc; p < &proc[NPROC]; p++) {
        acquire(&p->lock);
        if (p->state == RUNNABLE && p->in_queue == 0) {
            queue_at_end(p, p->queue_number);
            p->in_queue = 1;
        }
        // Handle processes that are relinquishing control for I/O
        if (p->state == SLEEPING) {
            // Re-insert the process into the same queue once it becomes runnable again
            queue_at_end(p, p->queue_number);
            p->in_queue = 1;
        }

        release(&p->lock);
    }

    struct proc *new = 0;
    int runnable_found = 0;
    for (int i = 0; i < NUM_QUEUES; i++) {
        while (queue_lengths[i] > 0) {
            new = dequeue_from_start(i);
            if (new->state == RUNNABLE) {
                // Reinsert back into the queue if it's runnable
                queue_at_start(new, i);
                runnable_found = 1;
                break;
            }
        }
        if (runnable_found) break;
    }

    // If a process was found, handle time slice and context switch
    if (new) {
        acquire(&new->lock);
        if (new->state == RUNNABLE) {
            // Check if it has used up its time slice
            if (new->ticks_in_queue >= time_slice[new->queue_number]) {
                dequeue_from_start(new->queue_number);
                if (new->queue_number < NUM_QUEUES - 1) {
                    new->queue_number++;  // Move to lower-priority queue
                }
                queue_at_end(new, new->queue_number);  // Insert at the end of the new queue
                new->ticks_in_queue = 0;  // Reset its time slice counter
            }

            // Handle round-robin at the lowest-priority queue
            if (new->queue_number == NUM_QUEUES - 1) {
                dequeue_from_start(new->queue_number);  // Remove from front
                queue_at_end(new, new->queue_number);  // Insert at the end
            }

            // Set the process to running and switch context
            new->state = RUNNING;
            c->proc = new;
            swtch(&c->context, &new->context);
            c->proc = 0;
        }
        release(&new->lock);
    } 

// #endif


#endif
#endif
#endif
  }
}

// Switch to scheduler.  Must hold only p->lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->noff, but that would
// break in the few places where a lock is held but
// there's no process.
void sched(void) {
  int intena;
  struct proc *p = myproc();

  if (!holding(&p->lock))
    panic("sched p->lock");
  if (mycpu()->noff != 1)
    panic("sched locks");
  if (p->state == RUNNING)
    panic("sched running");
  if (intr_get())
    panic("sched interruptible");

  intena = mycpu()->intena;
  swtch(&p->context, &mycpu()->context);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void yield(void) {
  struct proc *p = myproc();
  acquire(&p->lock);
    #ifdef MLFQ
    if(p->state == SLEEPING){
    #endif
      p->state = RUNNABLE;
    #ifdef MLFQ
    }
    #endif
  sched();
  release(&p->lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch to forkret.
void forkret(void) {
  static int first = 1;

  // Still holding p->lock from scheduler.
  release(&myproc()->lock);

  if (first) {
    // File system initialization must be run in the context of a
    // regular process (e.g., because it calls sleep), and thus cannot
    // be run from main().
    first = 0;
    fsinit(ROOTDEV);
  }

  usertrapret();
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void sleep(void *chan, struct spinlock *lk) {
  struct proc *p = myproc();

  // Must acquire p->lock in order to
  // change p->state and then call sched.
  // Once we hold p->lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup locks p->lock),
  // so it's okay to release lk.

  acquire(&p->lock); // DOC: sleeplock1
  release(lk);

  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  release(&p->lock);
  acquire(lk);
}

// Wake up all processes sleeping on chan.
// Must be called without any p->lock.
void wakeup(void *chan) {
  struct proc *p;

  for (p = proc; p < &proc[NPROC]; p++) {
    if (p != myproc()) {
      acquire(&p->lock);
      if (p->state == SLEEPING && p->chan == chan) {
        p->state = RUNNABLE;
      }
      release(&p->lock);
    }
  }
}

// Kill the process with the given pid.
// The victim won't exit until it tries to return
// to user space (see usertrap() in trap.c).
int kill(int pid) {
  struct proc *p;

  for (p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);
    if (p->pid == pid) {
      p->killed = 1;
      if (p->state == SLEEPING) {
        // Wake process from sleep().
        p->state = RUNNABLE;
      }
      release(&p->lock);
      return 0;
    }
    release(&p->lock);
  }
  return -1;
}

void setkilled(struct proc *p) {
  acquire(&p->lock);
  p->killed = 1;
  release(&p->lock);
}

int killed(struct proc *p) {
  int k;

  acquire(&p->lock);
  k = p->killed;
  release(&p->lock);
  return k;
}

// Copy to either a user address, or kernel address,
// depending on usr_dst.
// Returns 0 on success, -1 on error.
int either_copyout(int user_dst, uint64 dst, void *src, uint64 len) {
  struct proc *p = myproc();
  if (user_dst) {
    return copyout(p->pagetable, dst, src, len);
  } else {
    memmove((char *)dst, src, len);
    return 0;
  }
}

// Copy from either a user address, or kernel address,
// depending on usr_src.
// Returns 0 on success, -1 on error.
int either_copyin(void *dst, int user_src, uint64 src, uint64 len) {
  struct proc *p = myproc();
  if (user_src) {
    return copyin(p->pagetable, dst, src, len);
  } else {
    memmove(dst, (char *)src, len);
    return 0;
  }
}

// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void procdump(void) {
  static char *states[] = {
      [UNUSED] "unused",   [USED] "used",      [SLEEPING] "sleep ",
      [RUNNABLE] "runble", [RUNNING] "run   ", [ZOMBIE] "zombie"};
  struct proc *p;
  char *state;

  printf("\n");
  for (p = proc; p < &proc[NPROC]; p++) {
    if (p->state == UNUSED)
      continue;
    if (p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";

#ifdef LBS
    printf("%d %s %s %d %d", p->pid, state, p->name, p->tickets,
           p->arrival_time);
#else
#ifdef MLFQ
    // printf("%d %s %s %d  %d", p->pid, state, p->name, p->queue_number,
    //        ticks-p->ticks_in_current_queue);
        printf("%d %s %s %d %d %d running_time: %d  ticks: %d", p->pid, state, p->name, p->queue_number, ticks - p ->ticks_in_queue,p->position_in_queue,p->in_queue,ticks);

        // printf("pid : %d , queue_number : %d , ticks : %d , state : %s " , p->pid , p->queue_number , ticks ,state);
#else
    printf("%d %s %s", p->pid, state, p->name);
#endif
#endif
    printf("\n");
  }
}

// waitx
int waitx(uint64 addr, uint *wtime, uint *rtime) {
  struct proc *np;
  int havekids, pid;
  struct proc *p = myproc();

  acquire(&wait_lock);

  for (;;) {
    // Scan through table looking for exited children.
    havekids = 0;
    for (np = proc; np < &proc[NPROC]; np++) {
      if (np->parent == p) {
        // make sure the child isn't still in exit() or swtch().
        acquire(&np->lock);

        havekids = 1;
        if (np->state == ZOMBIE) {
          // Found one.
          pid = np->pid;
          *rtime = np->total_runtime;
          *wtime = np->time_ends - np->arrival_time - np->total_runtime;
          if (addr != 0 && copyout(p->pagetable, addr, (char *)&np->xstate,
                                   sizeof(np->xstate)) < 0) {
            release(&np->lock);
            release(&wait_lock);
            return -1;
          }
          freeproc(np);
          release(&np->lock);
          release(&wait_lock);
          return pid;
        }
        release(&np->lock);
      }
    }

    // No point waiting if we don't have any children.
    if (!havekids || p->killed) {
      release(&wait_lock);
      return -1;
    }

    // Wait for a child to exit.
    sleep(p, &wait_lock); // DOC: wait-sleep
  }
}

void update_time() {
  struct proc *p;
  for (p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);
    if (p->state == RUNNING) {
      p->time_ran++;
      p->total_runtime++;
       #ifdef MLFQ
      //  p->ticks_in_current_queue++;
      p->ticks_in_queue++;
       #endif
    } else if(p->state == SLEEPING){
      p->time_slept++;
      #ifdef MLFQ
      p->wait_time++;
      #endif
    }
    else if (p->state == RUNNABLE) {
      p->time_waited++;
    }
    release(&p->lock);
  }
}