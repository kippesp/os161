#include <stdio.h>

int main(int argc, const char* argv[])
{
  printf("%d arguments provided\n", argc);

  for (int i = 0; i < argc; i++) {
    printf("argv[0] = %s\n", argv[i]);
  }

  return 0;
}
