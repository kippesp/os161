#include <unistd.h>
#include <string.h>

#define STDOUT 1

int main(int argc, char **argv)
{
  (void)argc;
  (void)argv;

  const char* teststring = "Hello World!\n";

  write(STDOUT_FILENO, teststring, strlen(teststring));

  return 0;
}
