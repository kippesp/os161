#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* Example: /testbin/execv hello from the args test */

int main(int argc, char* const argv[])
{
  printf("Calling argv with %d args....\n", argc);
  int res = execv("/testbin/args", &argv[0]);

  printf("%d returned by execv() call to args.\n", res);

  return 0;
}
