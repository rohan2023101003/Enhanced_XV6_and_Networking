#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

char buf[512];

// void
// syscount()
// {
 
//   printf("Syscount is reached\n");
// }

int
main(int argc, char *argv[])
{

 if(argc < 3){
    printf("Usage: syscount <mask> command [args]\n");
    exit(1);
  }
  
  int mask = atoi(argv[1]);
  int pid = fork();
  if(pid < 0){
    printf("fork failed\n");
    exit(1);
  } else if(pid == 0){
    // Child process
    // for(int i=0;i<argc;i++){
    //   printf("%s\n",argv[i]);
    // }
    
    //give code to count syscalls made in child process also
    // getSysCount(mask);
    exec(argv[2], &argv[2]);
    printf("exec %s failed\n", argv[2]);
    exit(1);
  } else {
    // Parent process
    wait(0);
    uint64 count = getSysCount(mask,pid);
    
    // Find which system call was counted
    char *syscall_name;
    int syscall_num = 0;
    for(int i = 1; i < 32; i++) { // 1 to 31 syscalls
      if(mask == 1 << i) {
        syscall_num = i;
  
        break;
      }
    }
    if(syscall_num == 23 || syscall_num == 1){
      //the parent can only call either fork or syscount
      count++;
    }
    //if zero unknown
    // Map syscall number to name (you'll need to update this based on your syscall.h)
    switch(syscall_num) {
      case 1: syscall_name = "fork"; break;
      case 2: syscall_name = "exit"; break;
      case 3: syscall_name = "wait"; break;
      case 4: syscall_name = "pipe"; break;
      case 5: syscall_name = "read"; break;
      case 6: syscall_name = "kill"; break;
      case 7: syscall_name = "exec"; break;
      case 8: syscall_name = "fstat"; break;
      case 9: syscall_name = "chdir"; break;
      case 10: syscall_name = "dup"; break;
      case 11: syscall_name = "getpid"; break;
      case 12: syscall_name = "sbrk"; break;
      case 13: syscall_name = "sleep"; break;
      case 14: syscall_name = "uptime"; break;
      case 15: syscall_name = "open"; break;
      case 16: syscall_name = "write"; break;
      case 17: syscall_name = "mknod"; break;
      case 18: syscall_name = "unlink"; break;
      case 19: syscall_name = "link"; break;
      case 20: syscall_name = "mkdir"; break;
      case 21: syscall_name = "close"; break;
      case 22: syscall_name = "waitx"; break;
      case 23: syscall_name = "getSysCount"; break;
      // Add more cases for other system calls
      default: syscall_name = "unknown"; break;
    }
    
    printf("PID %d called %s %d times\n", pid, syscall_name, count);
  }
  
  exit(0);
}
