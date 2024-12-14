#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

char buf[512];

void
syscount()
{
 
  printf("Hello, world\n");
}

int
main(int argc, char *argv[])
{
  syscount();
  exit(0);
}
