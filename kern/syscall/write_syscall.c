#include <kern/errno.h>
#include <types.h>

#include <copyinout.h>
#include <current.h>
#include <filedescr.h>
#include <file_syscall.h>
#include <uio.h>
#include <vnode.h>
#include <synch.h>

int sys_write(int fh, const_userptr_t buf, size_t buflen,
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

  char* kbuf = (char*)kmalloc(buflen);
  KASSERT(kbuf);
  res = copyin(buf, (void*)kbuf, buflen);

  if (res) {
    kfree(kbuf);
    goto SYS_WRITE_ERROR;
  }

  // TODO: move this junk into a helper function

  struct uio wr_uio;
  struct iovec wr_uio_iov;

  wr_uio.uio_iov = &wr_uio_iov;
  wr_uio.uio_iovcnt = 1;
  wr_uio.uio_offset = fd->fd_pos;
  wr_uio.uio_resid = buflen;
  wr_uio.uio_segflg = UIO_SYSSPACE;
  wr_uio.uio_rw = UIO_WRITE;
  wr_uio.uio_space = NULL; /* kbuf is is kernel space */

  wr_uio_iov.iov_kbase = kbuf;
  wr_uio_iov.iov_len = buflen;

  lock_acquire(fd->fd_lock);
  res = VOP_WRITE(fd->fd_ofile, &wr_uio);

  /* Other than devices such as con:, update fd->fd_pos to reflect how much was
   * written.
   */

  *buflen_written = wr_uio.uio_offset;

  if (fd->fd_ofile->vn_fs) {
    fd->fd_pos += *buflen_written;
  }

  lock_release(fd->fd_lock);

  kfree(kbuf);

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
