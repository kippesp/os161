#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

int main(void)
{
  pid_t pid = fork();

  if (pid == -1) {
    printf("fork failed\n");
    exit(1);
  }

  if (pid == 0) {
    const char* filename = "/mytests/echo";
    const char* const args[] = {"echo", "Hello", "World,", "I'm",
                                "the",  "child", NULL};

    int res = execv(filename, (char* const*)args);

    printf("execv() returned %d--bummer!\n", res);
  }
  else {
    int status;
    pid_t pid_ret = waitpid(pid, &status, 0);
    printf("Child returned with %d to parent\n", status);
    if (pid_ret != pid) {
      printf("unmatched pid");
    }
  }

  return 0;
}
