#include <stdio.h>
#include <string.h>

int main(int argc, const char* argv[])
{
  int junk[] = {0xFEFEFEFE, 0xFEFEFEFE, 0xFEFEFEFE, 0xFEFEFEFE};

  (void)junk;

  printf("%d arguments provided\n", argc);

  for (int i = 0; i < argc; i++) {
    printf("argv[%d]@%p = (%p,%d bytes)%s\n", i, &argv[i], argv[i],
           (int)strlen(argv[i]), argv[i]);
  }

  if (argv[argc] == NULL) {
    printf("argv[%d]@%p is NULL\n", argc, &argv[argc]);
  }
  else {
    printf("argv[%d]@%p is not NULL\n", argc, &argv[argc]);
  }

  return 0;
}
