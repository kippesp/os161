#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <err.h>

/* this is to verify the lseek arguments are handled properly. */

int main(int argc, char** argv)
{
  (void)argc;
  (void)argv;

  puts("\noff_t ret = lseek(int fd, off_t pos, int whence)\n");

  int fd = 0x0A0B0C0D;
  off_t pos = 0xAABBCCDD11223344LL;
  int whence = 0x01020304;

  printf("fd = 0x%08x\n", fd);
  printf("pos = 0x%016llx\n", pos);
  printf("whence = 0x%08x\n", whence);

  puts("\ncalling....\n");

  off_t ret = lseek(fd, pos, whence);

  puts("\nreturned....\n");

  printf("ret = 0x%016llx\n", ret);

  return 0;
}
