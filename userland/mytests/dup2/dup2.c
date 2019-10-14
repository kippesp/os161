#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void)
{
  int logfd = open("dup2_logfile", O_WRONLY | O_CREAT, 0644);
  if (logfd == -1) {
    printf("open failed\n");
    exit(1);
  }

  /* note the sequence of parameter */
  int res = dup2(logfd, STDOUT_FILENO);
  close(logfd);

  printf("dup2 return: %d\n", res);

  /* now all print content will go to log file */
  printf("Hello.  dup2 wrote this (to the file).\n");
}
