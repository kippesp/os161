#include <kern/errno.h>
#include <types.h>

#include <copyinout.h>
#include <current.h>
#include <filedescr.h>
#include <file_syscall.h>
#include <uio.h>
#include <vnode.h>
#include <synch.h>

int sys_write(int fh, const_userptr_t ubuf, size_t buflen,
              ssize_t* buflen_written)
{
  int res = 0;
  struct filedesc* fd;

  res = get_fd(curproc, fh, &fd);

  if (res) {
    goto SYS_WRITE_ERROR;
  }

  if (buflen == 0) {
    *buflen_written = 0;
    goto SYS_WRITE_ERROR_FREE;
  }

  /* copy entire buffer into kernel space for writing */

  struct iovec wr_uio_iov;
  struct uio wr_uio;
  off_t initial_offset = fd->fd_pos;

  uio_uinit(&wr_uio_iov, &wr_uio, (void*)ubuf, buflen, initial_offset,
            UIO_WRITE);

  res = VOP_WRITE(fd->fd_ofile, &wr_uio);

  /* Other than devices such as con:, update fd->fd_pos to reflect how much was
   * written.
   */

  *buflen_written = wr_uio.uio_offset - initial_offset;

  lock_acquire(fd->fd_lock);
  if (fd->fd_ofile->vn_fs) {
    fd->fd_pos += *buflen_written;
  }
  lock_release(fd->fd_lock);

  if (res) {
    goto SYS_WRITE_ERROR;
  }

SYS_WRITE_ERROR_FREE:
  KASSERT(res == 0);
  return 0;

SYS_WRITE_ERROR:
  KASSERT(res != 0);
  return res;
}
