#include <kern/errno.h>
#include <types.h>

#include <kern/fcntl.h>

#include <copyinout.h>
#include <current.h>
#include <filedescr.h>
#include <file_syscall.h>
#include <proc.h>
#include <uio.h>
#include <vnode.h>
#include <synch.h>

int sys_write(int fh, const_userptr_t ubuf, size_t buflen,
              ssize_t* buflen_written)
{
  int res = 0;
  struct filedesc* fd;
  struct proc* p = curproc;

  lock_acquire(p->p_lk_syscall);

  res = get_fd(curproc, fh, &fd);

  if (res) {
    goto SYS_WRITE_ERROR;
  }

  /* check if writing is permitted */

  if (IS_OFLAGS_RO(fd->oflags)) {
    res = EBADF;
    goto SYS_WRITE_ERROR;
  }

  if (buflen == 0) {
    *buflen_written = 0;
    goto SYS_WRITE_ERROR_FREE;
  }

  /* copy entire buffer into kernel space for writing */

  lock_acquire(fd->fd_lock);

  struct iovec wr_uio_iov;
  struct uio wr_uio;
  off_t initial_offset = fd->fd_pos;

  uio_uinit(&wr_uio_iov, &wr_uio, (void*)ubuf, buflen, initial_offset,
            UIO_WRITE);

  res = VOP_WRITE(fd->fd_ofile, &wr_uio);

  if (res) {
    lock_release(fd->fd_lock);
    goto SYS_WRITE_ERROR;
  }

  /* Other than devices such as con:, update fd->fd_pos to reflect how much was
   * written.
   */

  *buflen_written = wr_uio.uio_offset - initial_offset;

  if (fd->fd_ofile->vn_fs) {
    fd->fd_pos += *buflen_written;
  }

  lock_release(fd->fd_lock);

SYS_WRITE_ERROR_FREE:
  lock_release(p->p_lk_syscall);
  KASSERT(lock_do_i_hold(p->p_lk_syscall) == 0);
  KASSERT(res == 0);
  return 0;

SYS_WRITE_ERROR:
  lock_release(p->p_lk_syscall);
  KASSERT(lock_do_i_hold(p->p_lk_syscall) == 0);
  KASSERT(res != 0);
  return res;
}
