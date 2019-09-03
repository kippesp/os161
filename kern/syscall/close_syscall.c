#include <kern/errno.h>

#include <types.h>

#include <current.h>
#include <filedescr.h>
#include <file_syscall.h>
#include <proc.h>
#include <synch.h>
#include <vfs.h>

int sys_close(int fh)
{
  int res = 0;
  struct filedesc* fd;
  struct proc* p = curproc;

  spinlock_acquire(&p->p_lock);

  res = get_fd(p, fh, &fd);

  if (res) {
    spinlock_release(&p->p_lock);
    goto SYS_CLOSE_ERROR;
  }

  /* check the descriptor's reference count */
  lock_acquire(fd->fd_lock);

  KASSERT(fd->fd_refcnt);
  fd->fd_refcnt--;

  /* if no more references, close the file and free the memory */
  if (fd->fd_refcnt == 0) {
    p->p_fdtable[fh] = NULL;
    spinlock_release(&p->p_lock);
    vfs_close(fd->fd_ofile);

    destroy_fd(fd);
  }
  else {
    lock_release(fd->fd_lock);
    spinlock_release(&p->p_lock);
  }

  goto SYS_CLOSE_ERROR_FREE;

SYS_CLOSE_ERROR_FREE:
  KASSERT(res == 0);
  return 0;

SYS_CLOSE_ERROR:
  KASSERT(res != 0);
  return res;
}
