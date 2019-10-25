#include <stdio.h>
#include <string.h>

int main(int argc, const char* argv[])
{
  for (int i = 1; i < argc; i++) {
    printf("%s ", argv[i]);
  }

  printf("\n");

  return 0;
}
