#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "syscall.h"
uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0; // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
    // struct proc *p = myproc();
    // p->syscall_count[SYS_fork]++;  // Increment the syscall count for fork()
    return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if (growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n)
  {
    if (killed(myproc()))
    {
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_waitx(void)
{
  uint64 addr, addr1, addr2;
  uint wtime, rtime;
  argaddr(0, &addr);
  argaddr(1, &addr1); // user virtual memory
  argaddr(2, &addr2);
  int ret = waitx(addr, &wtime, &rtime);
  struct proc *p = myproc();
  if (copyout(p->pagetable, addr1, (char *)&wtime, sizeof(int)) < 0)
    return -1;
  if (copyout(p->pagetable, addr2, (char *)&rtime, sizeof(int)) < 0)
    return -1;
  return ret;
}

uint64 
sys_getSysCount(void)
{
// printf("Syscount is reached\n");
  int mask;
  int pid;
  uint64 count = 0;
  struct proc *p = myproc();
  argint(0, &mask);
  argint(1, &pid);
  while(mask>>=1){
    count++;
  }
  struct proc *p1 ;
  for(p1 = proc; p1 < &proc[NPROC]; p1++){
    if(p1->pid == pid){
      p = p1;
      break;
    }
  }
  return p->syscall_count[count];
  // for(int i = 1; i < 32; i++) { //counting from 1 to 31 syscalls
  //   if(mask == 1 << i ) {
  //     count = p->syscall_count[i];
  //     // printf("reach insied , count is %d\n",count);
  //     break;
  //   }
  // }
  // //if zero no such syscall exists so return 0
  //  return count;
}
uint64
sys_sigalarm(void)
{
  int interval;
  uint64 handler;
  
  argint(0, &interval);
  argaddr(1, &handler);
   
  struct proc *p = myproc();
  p->alarm_interval = interval;
  p->alarm_handler = (void(*)())handler;
  p->ticks_count = 0;
  
  return 0;
}

uint64
   sys_sigreturn(void)
   {
     struct proc *p = myproc();
    // memmove(p->trapframe, p->alarm_trapframe, sizeof(*(p->trapframe)));
     if (!p->alarm_in_progress) {
       return -1;
     }
     // Restore the entire trapframe
     *(p->trapframe) = p->alarm_trapframe;
     p->alarm_in_progress = 0;
     usertrapret();
     return 0;
   }

uint64
sys_settickets(void)
{
#ifdef LBS
  int pid;
  argint(0, &pid);
  int new_tickets;
  argint(1, &new_tickets);
  int flag=0;
  struct proc *p;
    for (p = proc; p < &proc[NPROC]; p++) {
        acquire(&p->lock);
        if (p->pid == pid) {
          flag=1;
             break;
        }
        release(&p->lock);
    }
if(flag==0){
  printf("No process with pid %d exists\n", pid);
  return -1;
}
  // Ensure the number of tickets is at least 1
  if (new_tickets < 1)
    new_tickets = 1;
// ------->Processes with the Same Number of Tickets
    p->tickets = new_tickets;  // Set tickets to the new value
      // Pros:

      // Simplicity: Having a uniform number of tickets for processes simplifies the implementation and understanding of the scheduling algorithm.
      // Fairness: All processes are treated equally, reducing the chances of starvation for any single process.
      // Predictable Behavior: Processes are easier to analyze and predict since they have the same chance of being scheduled.
      // Cons:

      // Lack of Differentiation: This approach doesn't allow for prioritization based on the process's resource needs. All processes get equal treatment regardless of their nature (CPU-bound vs. I/O-bound).
      // Potential Inefficiency: In scenarios where some processes require more resources (like CPU-bound processes), equal ticket distribution can lead to inefficiencies, as they may not get the necessary resources to execute effectively.
// ---------->Processes with Different Numbers of Tickets
//p->tickets += new_tickets; 
        // Pros:

        // Prioritization: Processes can be prioritized based on their requirements. For example, CPU-bound processes can receive more tickets, increasing their chances of getting scheduled.
        // Dynamic Adjustment: This approach allows for more dynamic scheduling policies, adapting ticket counts based on process behavior, which can lead to better overall performance.
        // Reduced Starvation: With careful management of ticket distribution, you can mitigate starvation for lower-priority processes by adjusting their ticket counts over time.
        // Cons:

        // Complexity: Implementing and maintaining a system with varying ticket counts adds complexity to the scheduler, making it harder to manage and debug.
        // Potential for Starvation: If not carefully managed, lower-priority processes may suffer from starvation if higher-priority processes continuously accumulate tickets.
  release(&p->lock);
  return new_tickets;  // Return the updated ticket count
#endif
  return 0;
}