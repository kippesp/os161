#include <kern/errno.h>

#include <types.h>

#include <kern/fcntl.h>
#include <kern/stat.h>
#include <kern/unistd.h>

#include <copyinout.h>
#include <current.h>
#include <filedescr.h>
#include <file_syscall.h>
#include <limits.h>
#include <proc.h>
#include <synch.h>
#include <vnode.h>
#include <vfs.h>

int sys_open(const_userptr_t filename, int oflags, int* fh_ret)
{
  int res = 0;
  struct proc* p = curproc;

  /* check flags */

  int flags_masked_rw = oflags & O_ACCMODE;

  /* check only one mode flag is set */
  if ((flags_masked_rw != O_RDONLY) && (flags_masked_rw != O_WRONLY) &&
      (flags_masked_rw != O_RDWR)) {
    res = EINVAL;
    goto SYS_OPEN_ERROR;
  }

  int flags_masked =
      oflags & (O_ACCMODE | O_CREAT | O_EXCL | O_TRUNC | O_APPEND | O_NOCTTY);

  /* and nothing more */
  if (oflags != flags_masked) {
    res = EINVAL;
    goto SYS_OPEN_ERROR;
  }

  /* Sanitize some of the insane flag settings such as having O_APPEND set when
   * the operation is O_RDONLY.  Do thing saves from fussing with checks later.
   */
  if (oflags & O_RDONLY) {
    oflags &= ~(O_APPEND | O_TRUNC);
  }

  KASSERT(((oflags & O_EXCL) == 0) && "Not implemented");

  if (filename == NULL) {
    res = EFAULT;
    goto SYS_OPEN_ERROR;
  }

  char kfilename[__PATH_MAX + 1] = "\0";
  size_t kfilenamelen = 0;
  res = copyinstr(filename, kfilename, __PATH_MAX + 1, &kfilenamelen);
  if (res) {
    goto SYS_OPEN_ERROR;
  }

  if (strcmp(kfilename, "con:") == 0) {
    KASSERT(0 && "open() attempted for con:");
  }

  /*
   * allocate a new file descriptor; we can still fail if there is no place in
   * the process's file table
   */

  struct filedesc* fd = new_fd();

  if (fd == NULL) {
    res = ENOMEM;
    goto SYS_OPEN_ERROR;
  }

  int fh = -1;

  lock_acquire(p->p_lk_syscall);
  for (int i = STDERR_FILENO + 1; i < __OPEN_MAX; i++) {
    if (p->p_fdtable[i] == NULL) {
      fh = i;
      break;
    }
  }

  /* no space in the process's table? */
  if (fh == -1) {
    lock_release(p->p_lk_syscall);
    res = EMFILE;
    goto SYS_OPEN_ERROR;
  }

  /* Call vfs_open() to complete the open.  If successful the file descriptor
   * offset will be adjusted if the file was opened for writing with O_APPEND.
   */

  res = vfs_open(kfilename, oflags, 0666, &fd->fd_ofile);
  if (res) {
    destroy_fd(fd);
    lock_release(p->p_lk_syscall);
    goto SYS_OPEN_ERROR;
  }

  if (oflags & O_APPEND) {
    struct stat statbuf;
    res = VOP_STAT(fd->fd_ofile, &statbuf);

    if (res) {
      destroy_fd(fd);
      lock_release(p->p_lk_syscall);
      goto SYS_OPEN_ERROR;
    }

    fd->fd_pos = statbuf.st_size + 1;
  }

  if (oflags & O_TRUNC) {
    fd->fd_pos = 0;
  }

  fd->fd_refcnt++;
  fd->oflags = oflags;
  p->p_fdtable[fh] = fd;
  lock_release(p->p_lk_syscall);
  *fh_ret = fh;

  goto SYS_OPEN_ERROR_FREE;

SYS_OPEN_ERROR_FREE:
  KASSERT(lock_do_i_hold(p->p_lk_syscall) == 0);
  KASSERT(res == 0);
  return 0;

SYS_OPEN_ERROR:
  KASSERT(lock_do_i_hold(p->p_lk_syscall) == 0);
  KASSERT(res != 0);
  return res;
}
