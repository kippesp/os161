#include <unistd.h>
#include <string.h>

int main(int argc, char **argv)
{
  (void)argc;
  (void)argv;

  const char* s1 = "Writing to stderr\n";

  write(STDERR_FILENO, s1, strlen(s1));

  return 0;
}
