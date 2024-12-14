#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{

 if(argc < 3){
    printf("Usage: syscount <mask> command [args]\n");
    exit(1);
  }
  
  settickets(atoi(argv[1]),atoi(argv[2]));
  
  exit(0);
}
