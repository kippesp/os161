#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

int main(void)
{
  pid_t pid = fork();

  if (pid == -1) {
    printf("first fork failed");
    _exit(EXIT_FAILURE);
  }

  if (pid == 0) {
    pid_t mypid = getpid();
    printf("CPID(%d): Hello from the forked child process!\n", mypid);

    _exit(16 >> 2);
  }
  else {
    pid_t mypid = getpid();
    int status;
    waitpid(pid, &status, 0);
    printf("pid(%d): My child, pid=%d, has died with status %d!\n",
           mypid, pid, status);

    printf("CPID(%d): Hello from the parent process!\n", mypid);
  }

  printf("*** Exit fallthrough ***\n");

  return 0;
}
