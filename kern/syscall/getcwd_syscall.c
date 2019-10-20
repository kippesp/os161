#include <kern/errno.h>

#include <types.h>

#include <copyinout.h>
#include <current.h>
#include <file_syscall.h>
#include <proc.h>
#include <uio.h>
#include <vfs.h>

int sys___getcwd(userptr_t* buf, size_t buflen)
{
  int res = 0;
  struct proc* proc = curproc;

  lock_acquire(proc->p_lk_syscall);

  if (buf == NULL) {
    res = EFAULT;
    goto SYS_GETCWD_ERROR;
  }

  struct iovec iov;
  struct uio uio;

  uio_uinit(&iov, &uio, buf, buflen, 0, UIO_READ);
  res = vfs_getcwd(&uio);

  if (res) {
    goto SYS_GETCWD_ERROR;
  }

  goto SYS_GETCWD_ERROR_FREE;

SYS_GETCWD_ERROR_FREE:
  lock_release(proc->p_lk_syscall);
  KASSERT(res == 0);
  return 0;

SYS_GETCWD_ERROR:
  lock_release(proc->p_lk_syscall);
  KASSERT(res != 0);
  return res;
}
