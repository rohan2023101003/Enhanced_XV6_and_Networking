# xv6

## System Calls Implementation

This README explains the implementation of two new system calls in the xv6 operating system: getSysCount and sigalarm. The changes were made across various files to add these functionalities.

### 1. Gotta Count 'Em All (getSysCount)

The getSysCount system call counts the number of times a specific system call was called by a process. The corresponding user program syscount uses this system call to count and print the number of times a specified system call was invoked.

### How to Run
```
syscount <mask> command [args]
```
### Example:
```
$ syscount 66536 echo hello worlld
hello worlld
PID 4 called unknown 4 times
```

### 2. Wake Me Up When My Timer Ends (sigalarm and sigreturn)

The sigalarm system call sets up a periodic alarm that triggers a user-defined handler function after a specified number of CPU ticks. The sigreturn system call resets the process state to before the handler was called.


### How to Run
```
$ alarmtest
```
### Result 
```
test0 start
......................................alarm!
test0 passed
test1 start
....alarm!
....alarm!
...alarm!
....alarm!
...alarm!
....alarm!
...alarm!
....alarm!
...alarm!
....alarm!
test1 passed
test2 start
...........................................................!
test2 passed
test3 start
test3 passed
```
### Summary

- *getSysCount*: Counts the number of times a specific system call was called by a process and its children. Implemented changes in kernel/syscall.h, kernel/syscall.c, kernel/proc.h, kernel/proc.c, kernel/sysproc.c, user/syscount.c, user/user.h, user/usys.pl, and Makefile.
- *sigalarm and sigreturn*: Sets up a periodic alarm to trigger a user-defined handler function and resets the process state after the handler is called. Implemented changes in kernel/syscall.h, kernel/syscall.c, kernel/proc.h, kernel/proc.c, kernel/trap.c, user/alarmtest.c, user/user.h, user/usys.pl, and Makefile.

These changes add new functionalities to the xv6 operating system, allowing for more advanced process management and monitoring.

---
## Scheduling
The default scheduler of xv6 is round-robin-based. In this task, we have implemented four other scheduling policies and incorporated them in xv6. The kernel shall only use one scheduling policy which will be declared at compile time, with default being round-robin(RR) in case none is specified.

Also , to choose number of CPU's on which the scheduler run on specify it before running

### For LBS
```
make qemu SCHEDULER=LBS
```
### For x CPUs
```
make qemu SCHEDULER=LBS CPUS=x
```
### For MLFQ
We are Running MLFQ on only one CPU
```
make qemu SCHEDULER=MLFQ CPUS=1
```
#### Note :  
* If SCHEDULER and CPUS are not specified  then Round Robin is default scheduler and CPUS=2  by default.
* Refer to ```Report.pdf``` for implementation details.

---



