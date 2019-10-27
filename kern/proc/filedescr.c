#include <kern/errno.h>
#include <kern/limits.h>
#include <kern/unistd.h>

#include <filedescr.h>
#include <lib.h>
#include <proc.h>
#include <spinlock.h>
#include <synch.h>
#include <vfs.h>

/* Return 0 if the file handle is valid for the process or EBADF if outside the
 * range or is not assigned.
 */
static int check_assigned_fh(struct proc* p, int fh)
{
  KASSERT(p);

  if (!is_valid_fh(fh)) {
    return EBADF;
  }

  KASSERT(p->p_fdtable);

  if (p->p_fdtable[fh] == NULL) {
    return EBADF;
  }

  return 0;
}

bool is_valid_fh(int fh)
{
  return ((fh >= 0) && (fh < __OPEN_MAX));
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

struct filedesc** dup_fdtable(struct filedesc** src)
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

/* Called by waitpid to cleanup from exited child process.  Any descriptors
 * with no remaining references are closed before deallocation. */
void undup_fdtable(struct filedesc** fdtable)
{
  KASSERT(fdtable);

  for (int fh = 0; fh < __OPEN_MAX; fh++) {
    if (fdtable[fh]) {
      lock_acquire(fdtable[fh]->fd_lock);
      KASSERT(fdtable[fh]->fd_refcnt >= 1);
      fdtable[fh]->fd_refcnt--;

      /* close the unreferenced file */
      if (fdtable[fh]->fd_refcnt == 0) {
        vfs_close(fdtable[fh]->fd_ofile);
        lock_release(fdtable[fh]->fd_lock);
        destroy_fd(fdtable[fh]);
      }
      else {
        lock_release(fdtable[fh]->fd_lock);
      }

      fdtable[fh] = NULL;
    }
  }
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

/* Allocate a new file descriptor at a specific file handle */
int new_fd_from_fh(struct proc* p, int fh, struct filedesc** fd)
{
  KASSERT(p);
  KASSERT(fd);

  if (!is_valid_fh(fh)) {
    return EBADF;
  }

  if (p->p_fdtable[fh] != NULL) {
    return EBADF;
  }

  *fd = new_fd();

  if (*fd == NULL) {
    return ENOMEM;
  }

  p->p_fdtable[fh] = *fd;

  return 0;
}

/* Clean up unused file descriptor at file close or process deallocation. */
void destroy_fd(struct filedesc* fd)
{
  KASSERT(lock_do_i_hold(fd->fd_lock) == 0);
  KASSERT(fd->fd_refcnt == 0);

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
