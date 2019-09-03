#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char** argv)
{
  (void)argc;
  (void)argv;

  int res = open("file_simpleopen", O_WRONLY | O_CREAT);

  if (res == -1)
  {
    printf("open failed, error=%d\n", errno);
    return 1;
  }

  return 0;
}
