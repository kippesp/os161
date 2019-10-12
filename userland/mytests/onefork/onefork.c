#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

int main(void)
{
  pid_t pid = fork();

  struct {
    /* char byte; */
    int unaligned_status;
  } statuss;

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
    int res = waitpid(pid, &statuss.unaligned_status, 0);
    if (res == -1) {
      printf("waitpid failed with error %d\n", errno);
    }
    else {
      printf("waitpid did not fail\n");
      printf("pid(%d): My child, pid=%d, has died with status %d!\n", mypid,
             pid, statuss.unaligned_status);
    }

    printf("CPID(%d): Hello from the parent process!\n", mypid);
  }

  printf("*** Exit fallthrough ***\n");

  return 0;
}
