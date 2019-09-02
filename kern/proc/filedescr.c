#include <kern/errno.h>
#include <kern/limits.h>

#include <filedescr.h>
#include <lib.h>
#include <proc.h>

/* Return the file descriptor, fd, given a process's file handle, fh */
int get_fd(struct proc* p, int fd, struct filedesc** fh)
{
  KASSERT(fh);

  if ((fd < 0) || (fd > __OPEN_MAX - 1)) {
    return EBADF;
  }

  KASSERT(p->p_fdtable);

  if (p->p_fdtable[fd] == NULL) {
    return EBADF;
  }

  /* The file descriptor is valid (but may not be opened). */

  *fh = p->p_fdtable[fd];

  return 0;
}

int get_free_fh(struct proc* p)
{
  // TODO
  (void)p;

  return 0;
}

int get_fh(struct proc* p, struct filedesc* fd)
{
  // TODO
  (void)p;
  (void)fd;

  return 0;
}

int free_fh(struct proc* p, int fh)
{
  // TODO
  (void)p;
  (void)fh;

  return 0;
}
