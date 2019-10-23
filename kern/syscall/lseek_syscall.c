#include <types.h>

#include <kern/errno.h>
#include <kern/stat.h>
#include <kern/seek.h>

#include <current.h>
#include <file_syscall.h>
#include <filedescr.h>
#include <synch.h>
#include <vnode.h>

off_t sys_lseek(int fh, off_t pos, int whence, off_t* new_pos_ret)
{
  int res = 0;

  /* check whence */

  int whence_masked = whence & (SEEK_SET | SEEK_CUR | SEEK_END);

  /* just one flag is set */
  if ((whence_masked != SEEK_SET) && (whence_masked != SEEK_CUR) &&
      (whence_masked != SEEK_END)) {
    res = EINVAL;
    goto SYS_LSEEK_ERROR;
  }

  /* and nothing more */
  if (whence_masked != whence) {
    res = EINVAL;
    goto SYS_LSEEK_ERROR;
  }

  struct filedesc* fd;

  res = get_fd(curproc, fh, &fd);

  if (res) {
    goto SYS_LSEEK_ERROR;
  }

  /* can't lseek on some things */
  if (!VOP_ISSEEKABLE(fd->fd_ofile)) {
    res = ESPIPE;
    goto SYS_LSEEK_ERROR;
  }

  /* calculate the desired new position */

  lock_acquire(fd->fd_lock);

  off_t new_pos = fd->fd_pos;

  if (whence == SEEK_SET) {
    new_pos = pos;
  }
  else if (whence == SEEK_CUR) {
    new_pos += pos;
  }
  else { /* SEEK_END */
    struct stat statbuf;
    res = VOP_STAT(fd->fd_ofile, &statbuf);

    if (res) {
      lock_release(fd->fd_lock);
      goto SYS_LSEEK_ERROR;
    }

    new_pos = statbuf.st_size + pos;
  }

  if (new_pos < 0) {
    res = EINVAL;
    lock_release(fd->fd_lock);
    goto SYS_LSEEK_ERROR;
  }

  /* update the new position */
  fd->fd_pos = new_pos;
  *new_pos_ret = new_pos;

  lock_release(fd->fd_lock);

  goto SYS_LSEEK_ERROR_FREE;

SYS_LSEEK_ERROR_FREE:
  KASSERT(res == 0);
  return 0;

SYS_LSEEK_ERROR:
  KASSERT(res != 0);
  return res;
}
