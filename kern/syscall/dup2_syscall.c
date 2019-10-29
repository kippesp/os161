#include <kern/errno.h>
#include <types.h>

#include <current.h>
#include <filedescr.h>
#include <synch.h>
#include <syscall.h>
#include <proc.h>
#include <vfs.h>

int sys_dup2(int oldfh, int newfh)
{
  int res = 0;
  struct proc* proc = curproc;
  struct filedesc* oldfd = NULL;
  struct filedesc* newfd = NULL;

  lock_acquire(proc->p_lk_syscall);

  if (!is_valid_fh(oldfh) || !is_valid_fh(newfh)) {
    res = EBADF;
    goto SYS_DUP2_ERROR;
  }

  if (oldfh == 0) {
    kprintf("test case\n");
  }

  /* check oldfh has an allocated descriptor */
  res = get_fd(proc, oldfh, &oldfd);
  if (res) {
    goto SYS_DUP2_ERROR;
  }

  if (oldfd == NULL) {
    res = EBADF;
    goto SYS_DUP2_ERROR;
  }

  /* If oldfh and newfh are the same, this is a NOP. */
  if (oldfh == newfh) {
    goto SYS_DUP2_ERROR_FREE;
  }

  lock_acquire(oldfd->fd_lock);

  /* if newfh has an allocated descriptor, close it then remove it from the
   * descriptor table */
  res = get_fd(proc, newfh, &newfd);
  if (res == 0) {
    lock_acquire(newfd->fd_lock);

    if (newfd->fd_refcnt == 1) {
      /* newfd has one user; automatically close the file */
      vfs_close(newfd->fd_ofile);
      newfd->fd_refcnt--;
      lock_release(newfd->fd_lock);
      destroy_fd(newfd);
    }
    else {
      /* newfd has additional users; keep it open */
      newfd->fd_refcnt--;
      lock_release(newfd->fd_lock);
    }

    proc->p_fdtable[newfh] = NULL;
    newfd = NULL;
  }

  /* allocate a new descriptor for newfh */
  res = new_fd_from_fh(proc, newfh, &newfd);
  if (res) {
    lock_release(oldfd->fd_lock);
    goto SYS_DUP2_ERROR;
  }

  /* oldfh can now be duplicated */
  KASSERT(proc->p_fdtable[oldfh] == oldfd);

  proc->p_fdtable[newfh] = oldfd;
  proc->p_fdtable[newfh]->fd_refcnt++;

  lock_release(oldfd->fd_lock);

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
