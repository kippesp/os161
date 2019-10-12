#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

int main(void)
{
  pid_t pid_1 = fork();

  if (pid_1 == -1) {
    printf("first fork failed");
    _exit(EXIT_FAILURE);
  }

  if (pid_1 == 0) {
    pid_t mypid = getpid();
    printf("CPID(%d): Hello from the first forked child process!\n", mypid);

    _exit(16 >> 2);
  }
  else {
    pid_t mypid = getpid();
    int status;
    waitpid(pid_1, &status, 0);
    printf("pid_1(%d): My first child, pid_1=%d, has died with status %d!\n",
           mypid, pid_1, status);

    printf("CPID(%d): Hello from the parent process!\n", mypid);

    printf("CPID(%d): Forking second child\n", mypid);
  }

  pid_t pid_2 = fork();

  if (pid_2 == -1) {
    printf("second fork failed");
    _exit(EXIT_FAILURE);
  }

  if (pid_2 == 0) {
    pid_t mypid = getpid();
    printf("CPID(%d): Hello from the second forked child process!\n", mypid);

    _exit(32 >> 2);
  }
  else {
    pid_t mypid = getpid();
    int status;
    waitpid(pid_2, &status, 0);
    remove("bogus");
    printf("pid_2(%d): My second child, pid_2=%d, has died with status %d!\n",
           mypid, pid_2, status);

    printf("CPID(%d): Hello from the parent process!\n", mypid);
  }

  printf("*** Exit fallthrough ***\n");

  return 0;
}
