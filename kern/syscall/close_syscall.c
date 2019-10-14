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

  lock_acquire(p->p_lk_syscall);

  res = get_fd(p, fh, &fd);

  if (res) {
    lock_release(p->p_lk_syscall);
    goto SYS_CLOSE_ERROR;
  }

  /* decrease the descriptor's reference count by 1 */

  lock_acquire(fd->fd_lock);

  KASSERT(fd->fd_refcnt);
  fd->fd_refcnt--;

  /* if no more references, close the file and free the memory */

  if (fd->fd_refcnt == 0) {
    p->p_fdtable[fh] = NULL;
    vfs_close(fd->fd_ofile);
    destroy_fd(fd);
  }
  else {
    /* A child thread must hold a reference.  Don't close the file, but remove
     * it from my file table all the same. */
    p->p_fdtable[fh] = NULL;
    lock_release(fd->fd_lock);
  }

  lock_release(p->p_lk_syscall);

  goto SYS_CLOSE_ERROR_FREE;

SYS_CLOSE_ERROR_FREE:
  KASSERT(lock_do_i_hold(p->p_lk_syscall) == 0);
  KASSERT(res == 0);
  return 0;

SYS_CLOSE_ERROR:
  KASSERT(lock_do_i_hold(p->p_lk_syscall) == 0);
  KASSERT(res != 0);
  return res;
}
