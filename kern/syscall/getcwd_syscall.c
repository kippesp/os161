#include <kern/errno.h>

#include <types.h>

#include <copyinout.h>
#include <file_syscall.h>
#include <uio.h>
#include <vfs.h>

int __getcwd(userptr_t* buf, size_t buflen)
{
  (void)buflen;
  int res = 0;

  KASSERT(0 && "__getcwd() not tested");

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
  KASSERT(res == 0);
  return 0;

SYS_GETCWD_ERROR:
  KASSERT(res != 0);
  return res;
}
