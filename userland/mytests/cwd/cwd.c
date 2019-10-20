#include <unistd.h>
#include <limits.h>
#include <stdio.h>
#include <errno.h>

int main(int argc, const char* argv[])
{
  for (int i = 1; i < argc; i++)
  {
    printf("changing to: %s\n", argv[i]);
    int res = chdir(argv[i]);
    printf("result: %d\n\n", res);

    char p[PATH_MAX];
    char* rstring;
    rstring = getcwd(p, sizeof(p));

    if (rstring) {
      printf("getcwd(): %s\n", p);
    } else {
      printf("getcwd() failed: %d\n", errno);
    }
  }

  return 0;
}
