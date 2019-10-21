#include <kern/errno.h>

#include <types.h>

#include <kern/fcntl.h>
#include <kern/stat.h>

#include <copyinout.h>
#include <current.h>
#include <filedescr.h>
#include <file_syscall.h>
#include <proc.h>
#include <synch.h>
#include <uio.h>
#include <vnode.h>

int sys_read(int fh, userptr_t ubuf, size_t buflen, ssize_t* buflen_read)
{
  int res = 0;
  struct filedesc* fd;
  struct proc* p = curproc;

  lock_acquire(p->p_lk_syscall);

  res = get_fd(curproc, fh, &fd);

  if (res) {
    goto SYS_READ_ERROR;
  }

  if (fd->oflags & O_WRONLY) {
    res = EBADF;
    goto SYS_READ_ERROR;
  }

  /* get the file size to size the read buffer appropriately */

  lock_acquire(fd->fd_lock);

  struct stat statbuf;
  res = VOP_STAT(fd->fd_ofile, &statbuf);

  if (res) {
    lock_release(fd->fd_lock);
    goto SYS_READ_ERROR;
  }

  struct iovec rd_uio_iov;
  struct uio rd_uio;

  uio_uinit(&rd_uio_iov, &rd_uio, ubuf, buflen, fd->fd_pos, UIO_READ);

  res = VOP_READ(fd->fd_ofile, &rd_uio);

  if (res) {
    lock_release(fd->fd_lock);
    goto SYS_READ_ERROR;
  }

  *buflen_read = rd_uio.uio_offset - fd->fd_pos;
  fd->fd_pos = rd_uio.uio_offset;

  lock_release(fd->fd_lock);

  goto SYS_READ_ERROR_FREE;

SYS_READ_ERROR_FREE:
  KASSERT(res == 0);
  lock_release(p->p_lk_syscall);
  KASSERT(lock_do_i_hold(p->p_lk_syscall) == 0);
  return 0;

SYS_READ_ERROR:
  KASSERT(res != 0);
  lock_release(p->p_lk_syscall);
  KASSERT(lock_do_i_hold(p->p_lk_syscall) == 0);
  return res;
}
