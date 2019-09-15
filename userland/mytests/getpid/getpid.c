#include <stdio.h>
#include <unistd.h>

int main(int argc, char** argv)
{
  (void)argc;
  (void)argv;

  pid_t mypid = getpid();

  printf("My pid is %d\n", mypid);

  return 0;
}
