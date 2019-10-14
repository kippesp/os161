#include <kern/errno.h>
#include <types.h>

#include <current.h>
#include <filedescr.h>
#include <file_syscall.h>
#include <synch.h>
#include <proc.h>
#include <vfs.h>

int sys_dup2(int sourcefh, int newfh)
{
  int res = 0;
  struct proc* proc = curproc;
  struct filedesc* sourcefd = NULL;
  struct filedesc* newfd = NULL;

  lock_acquire(proc->p_lk_syscall);

  /* Same? */
  if (sourcefh == newfh) {
    /* The two handles are the same.  But were they also valid? */
    res = get_fd(proc, sourcefh, &sourcefd);
    if (res) {
      /* nope */
      goto SYS_DUP2_ERROR;
    }

    goto SYS_DUP2_ERROR_FREE;
  }

  res = get_fd(proc, sourcefh, &sourcefd);
  if (res) {
    goto SYS_DUP2_ERROR;
  }

  lock_acquire(sourcefd->fd_lock);

  /* check newfh is a valid handle; if so, does it already have an existing
   * file descriptor by not being NULL? */
  res = get_fd(proc, newfh, &newfd);

  if (newfd == NULL) {
    /* newfd doesn't exist, but newfh isn't valid either */
    if (res == EBADF) {
      lock_release(sourcefd->fd_lock);
      goto SYS_DUP2_ERROR;
    }
  }
  else {
    KASSERT(newfd);
    KASSERT(proc->p_fdtable[newfh] == newfd);
    lock_acquire(newfd->fd_lock);
    if (newfd->fd_refcnt == 1) {
      /* newfd has one user */
      vfs_close(newfd->fd_ofile);
      newfd->fd_refcnt--;
      destroy_fd(newfd);
    }
    else {
      /* newfd has additional users */
      newfd->fd_refcnt--;
      lock_release(newfd->fd_lock);
    }

    proc->p_fdtable[newfh] = NULL;
    newfd = NULL;
  }

  /* Any existing "newfh" is now closed or dereferenced; sourcefh can now be
   * duplicated */
  KASSERT(proc->p_fdtable[newfh] == NULL);
  KASSERT(proc->p_fdtable[sourcefh] == sourcefd);

  proc->p_fdtable[newfh] = sourcefd;
  proc->p_fdtable[newfh]->fd_refcnt++;

  lock_release(sourcefd->fd_lock);

  KASSERT(res == 0);

  goto SYS_DUP2_ERROR_FREE;

SYS_DUP2_ERROR_FREE:
  lock_release(proc->p_lk_syscall);
  KASSERT(res == 0);
  return 0;

SYS_DUP2_ERROR:
  lock_release(proc->p_lk_syscall);
  KASSERT(res != 0);
  return res;
}
