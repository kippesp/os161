#include <kern/errno.h>
#include <kern/limits.h>
#include <kern/unistd.h>

#include <filedescr.h>
#include <lib.h>
#include <proc.h>
#include <spinlock.h>
#include <synch.h>

/* Return 0 if the file handle is valid for the process or EBADF if outside the
 * range or is not assigned.
 */
static int check_assigned_fh(struct proc* p, int fh)
{
  KASSERT(p);

  if ((fh < 0) || (fh > __OPEN_MAX - 1)) {
    return EBADF;
  }

  KASSERT(p->p_fdtable);

  if (p->p_fdtable[fh] == NULL) {
    return EBADF;
  }

  return 0;
}

struct filedesc** init_fdtable(void)
{
  struct filedesc** fdtable = kmalloc(sizeof(struct filedesc*) * __OPEN_MAX);
  if (fdtable == NULL) {
    return NULL;
  }

  for (int i = 0; i < __OPEN_MAX; i++) {
    fdtable[i] = NULL;
  }

  return fdtable;
}

struct filedesc** copy_fdtable(struct filedesc** src)
{
  KASSERT(src);

  struct filedesc** fdtable = init_fdtable();

  if (!fdtable) {
    return NULL;
  }

  for (int i = 0; i < __OPEN_MAX; i++) {
    if (src[i]) {
      KASSERT(src[i]->fd_refcnt >= 1);
      src[i]->fd_refcnt++;

      fdtable[i] = src[i];
    }
  }

  return fdtable;
}

/* Return the file descriptor, fd, given a process's file handle, fh */
int get_fd(struct proc* p, int fh, struct filedesc** fd)
{
  int res = check_assigned_fh(p, fh);

  if (res) {
    return res;
  }

  /* The file descriptor is valid (but may not be opened). */

  *fd = p->p_fdtable[fh];

  return 0;
}

/* Allocates and initializes a new file descriptor. */
struct filedesc* new_fd(void)
{
  struct filedesc* fd = kmalloc(sizeof(struct filedesc));
  memset(fd, 0x00, sizeof(struct filedesc));
  fd->fd_lock = lock_create("fd");

  return fd;
}

/* I'm called at the close */
void destroy_fd(struct filedesc* fd)
{
  KASSERT(lock_do_i_hold(fd->fd_lock));
  KASSERT(fd->fd_refcnt == 0);

  lock_release(fd->fd_lock);
  lock_destroy(fd->fd_lock);

  kfree(fd);
}

/* Find an available file handle and return a locked, newly initialized file
 * descriptor. */
int new_fh(struct proc* p, int* fh)
{
  KASSERT(p);

  int fh_ret = -1;
  struct filedesc* fd = new_fd();

  if (fd == NULL) {
    return ENOMEM;
  }

  spinlock_acquire(&p->p_lock);
  for (int i = STDERR_FILENO + 1; i < __OPEN_MAX; i++) {
    if (p->p_fdtable[i] == NULL) {
      fh_ret = i;
      p->p_fdtable[fh_ret] = fd;
      break;
    }
  }
  spinlock_release(&p->p_lock);

  lock_acquire(fd->fd_lock);
  *fh = fh_ret;

  return 0;
}

int get_fh(struct proc* p, struct filedesc* fd)
{
  // TODO
  (void)p;
  (void)fd;

  KASSERT(p);
  KASSERT(fd);
  KASSERT(0 && "Not implemented");

  return 0;
}
