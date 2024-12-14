#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

struct spinlock tickslock;
uint ticks;

extern char trampoline[], uservec[], userret[];

// in kernelvec.S, calls kerneltrap().
void kernelvec();

extern int devintr();

void trapinit(void)
{
  initlock(&tickslock, "time");
}

// set up to take exceptions and traps while in the kernel.
void trapinithart(void)
{
  w_stvec((uint64)kernelvec);
}

//
// handle an interrupt, exception, or system call from user space.
// called from trampoline.S
//
void usertrap(void)
{
  int which_dev = 0;

  if ((r_sstatus() & SSTATUS_SPP) != 0)
    panic("usertrap: not from user mode");

  // send interrupts and exceptions to kerneltrap(),
  // since we're now in the kernel.
  w_stvec((uint64)kernelvec);

  struct proc *p = myproc();

  // save user program counter.
  p->trapframe->epc = r_sepc();

  if (r_scause() == 8)
  {
    // system call

    if (killed(p))
      exit(-1);

    // sepc points to the ecall instruction,
    // but we want to return to the next instruction.
    p->trapframe->epc += 4;

    // an interrupt will change sepc, scause, and sstatus,
    // so enable only now that we're done with those registers.
    intr_on();

    syscall();
  }
  else if ((which_dev = devintr()) != 0)
  {
    // ok
  }
  else
  {
    printf("usertrap(): unexpected scause %p pid=%d\n", r_scause(), p->pid);
    printf("            sepc=%p stval=%p\n", r_sepc(), r_stval());
    setkilled(p);
  }

    if (killed(p))
    exit(-1);
    #ifdef MLFQ
      struct proc* p_new;
      for (p_new = proc; p_new < &proc[NPROC]; p_new++) {
          if (p_new != 0) {
              if (p_new->state == RUNNABLE && p_new->in_queue == 1) {
                //wait itme = 30 ticks (can be chnaged accccordingly )
                  if (p_new->wait_time >= 30) {
                      // Remove process from its current queue
                      dequeue_at_a_position(p_new, p_new->queue_number);
                      
                      // Mark the process as no longer in the queue
                      p_new->in_queue = 0;
                      
                      // Reset wait time for the process
                      p_new->wait_time = 0;
                      
                      // Check if the process can be promoted (increase priority)
                      if (p_new->queue_number) {
                          p_new->queue_number--; // Decrease queue number (increase priority)
                          
                          // Insert the process at the end of the new queue
                          queue_at_end(p_new, p_new->queue_number);
                      }
                  }
              }
          }
      }

    #endif
  // give up the CPU if this is a timer interrupt.
  if (which_dev == 2 && p!=0){
      // timer interrupt
    if(p->alarm_interval > 0 && !p->alarm_in_progress) {
      p->ticks_count++;
      if(p->ticks_count >= p->alarm_interval) {
        p->ticks_count = 0;
        p->alarm_in_progress = 1;
        *(p->copy_of_tapeframe) = *(p->trapframe);
        // Save current trapframe
        memmove(&p->alarm_trapframe, p->trapframe, sizeof(struct trapframe));
        
        // Set up trapframe to call alarm_handler
        p->trapframe->epc = (uint64)p->alarm_handler;
      }
    }
   
    #ifdef RR
    yield();
    #elif LBS
    yield();
    #elif MLFQ
        int time_in_queue = p->ticks_in_queue;
        int assigned_ticks = time_slice[p->queue_number];
        // Check if the process has exceeded its allotted time slice
        if (time_in_queue >= assigned_ticks) {
            // If there are processes in the current queue
            if (queue_lengths[p->queue_number] > 0) {
                p->in_queue = 0;  // Mark the process as no longer in the queue
                dequeue_from_start(p->queue_number);  // Remove the process from the queue
                // dequeue_at_a_position(p_new, p_new->queue_number);
            }
            // If the process is not already in the lowest priority queue
            if (p->queue_number < NUM_QUEUES - 1) {
                p->queue_number++;  // Increase the queue number (lower priority)
                queue_at_end(p, p->queue_number);  // Re-add the process to the end of the new queue
            }
            // Reset the process' runtime and wait time
            p->ticks_in_queue = 0;
            p->wait_time = 0;
        }

    #endif    
  }
  usertrapret();
}

//
// return to user space
//
void usertrapret(void)
{
  struct proc *p = myproc();

  // we're about to switch the destination of traps from
  // kerneltrap() to usertrap(), so turn off interrupts until
  // we're back in user space, where usertrap() is correct.
  intr_off();

  // send syscalls, interrupts, and exceptions to uservec in trampoline.S
  uint64 trampoline_uservec = TRAMPOLINE + (uservec - trampoline);
  w_stvec(trampoline_uservec);

  // set up trapframe values that uservec will need when
  // the process next traps into the kernel.
  p->trapframe->kernel_satp = r_satp();         // kernel page table
  p->trapframe->kernel_sp = p->kstack + PGSIZE; // process's kernel stack
  p->trapframe->kernel_trap = (uint64)usertrap;
  p->trapframe->kernel_hartid = r_tp(); // hartid for cpuid()

  // set up the registers that trampoline.S's sret will use
  // to get to user space.

  // set S Previous Privilege mode to User.
  unsigned long x = r_sstatus();
  x &= ~SSTATUS_SPP; // clear SPP to 0 for user mode
  x |= SSTATUS_SPIE; // enable interrupts in user mode
  w_sstatus(x);

  // set S Exception Program Counter to the saved user pc.
  w_sepc(p->trapframe->epc);

  // tell trampoline.S the user page table to switch to.
  uint64 satp = MAKE_SATP(p->pagetable);

  // jump to userret in trampoline.S at the top of memory, which
  // switches to the user page table, restores user registers,
  // and switches to user mode with sret.
  uint64 trampoline_userret = TRAMPOLINE + (userret - trampoline);
  ((void (*)(uint64))trampoline_userret)(satp);
}

// interrupts and exceptions from kernel code go here via kernelvec,
// on whatever the current kernel stack is.
void kerneltrap() {
    int which_dev = 0;
    uint64 sepc = r_sepc();
    uint64 sstatus = r_sstatus();
    uint64 scause = r_scause();
    // struct proc *p = myproc();

    // Check if the trap came from supervisor mode
    if ((sstatus & SSTATUS_SPP) == 0)
        panic("kerneltrap: not from supervisor mode");

    // Ensure interrupts are disabled
    if (intr_get() != 0)
        panic("kerneltrap: interrupts enabled");
    // Handle device interrupts
    if ((which_dev = devintr()) == 0) {
        printf("scause %p\n", scause);
        printf("sepc=%p stval=%p\n", r_sepc(), r_stval());
        panic("kerneltrap");
    }
  // Handle timer interrupts

    // if (which_dev == 2 && p != 0 && p->state == RUNNING) {
    //     if (p->killed) {
    //         exit(-1);  // Exit if the process is killed
    //     }
    //     yield();  // Give up the CPU if the process is running
    // }
    // // Restore trap registers for use by kernelvec.S's sepc instruction
    w_sepc(sepc);
    w_sstatus(sstatus);
}

void clockintr()
{
  acquire(&tickslock);
  ticks++;
  #ifdef MLFQ
     boost_ticks++;
     struct proc *p;
            for (p = proc; p < &proc[NPROC]; p++) {
                printf("Time %d: Process %d priority %d\n" , ticks , p->pid,p->queue_number);
              }
  #endif
  update_time();
  wakeup(&ticks);
  release(&tickslock);
  #ifdef LBS
  struct proc *p =myproc();
  if(p!=0 && p->state == RUNNING){
    yield();
  }
  #endif
}

// check if it's an external interrupt or software interrupt,
// and handle it.
// returns 2 if timer interrupt,
// 1 if other device,
// 0 if not recognized.
int devintr()
{
  uint64 scause = r_scause();

  if ((scause & 0x8000000000000000L) &&
      (scause & 0xff) == 9)
  {
    // this is a supervisor external interrupt, via PLIC.

    // irq indicates which device interrupted.
    int irq = plic_claim();

    if (irq == UART0_IRQ)
    {
      uartintr();
    }
    else if (irq == VIRTIO0_IRQ)
    {
      virtio_disk_intr();
    }
    else if (irq)
    {
      printf("unexpected interrupt irq=%d\n", irq);
    }

    // the PLIC allows each device to raise at most one
    // interrupt at a time; tell the PLIC the device is
    // now allowed to interrupt again.
    if (irq)
      plic_complete(irq);

    return 1;
  }
  else if (scause == 0x8000000000000001L)
  {
    // software interrupt from a machine-mode timer interrupt,
    // forwarded by timervec in kernelvec.S.

    if (cpuid() == 0)
    {
      clockintr();
    }

    // acknowledge the software interrupt by clearing
    // the SSIP bit in sip.
    w_sip(r_sip() & ~2);

    return 2;
  }
  else
  {
    return 0;
  }
}
