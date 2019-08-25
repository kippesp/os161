#include <proc.h>
#include <filedescr.h>

int get_free_fh(struct proc* p)
{
  (void)p;

  return 0;
}

int get_fh(struct proc* p, struct filedesc* fd)
{
  (void)p;
  (void)fd;

  return 0;
}

int free_fh(struct proc* p, int fh)
{
  (void)p;
  (void)fh;

  return 0;
}
