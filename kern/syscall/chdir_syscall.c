#include <kern/errno.h>

#include <types.h>

#include <copyinout.h>
#include <file_syscall.h>
#include <limits.h>
#include <vfs.h>

int sys_chdir(const_userptr_t upathname)
{
  int res = 0;

  KASSERT(0 && "chdir() not tested");

  if (upathname == NULL) {
    res = EFAULT;
    goto SYS_CHDIR_ERROR;
  }

#if 0
  // Not sure if I need better protection
  // this is from kern/proc/proc.c

  /*
   * Lock the current process to copy its current directory.
   * (We don't need to lock the new process, though, as we have
   * the only reference to it.)
   */
  spinlock_acquire(&curproc->p_lock);
  if (curproc->p_cwd != NULL) {
    VOP_INCREF(curproc->p_cwd);
    newproc->p_cwd = curproc->p_cwd;
  }
  spinlock_release(&curproc->p_lock);
#endif

  char pathname[__PATH_MAX + 1] = "\0";
  size_t pathnamelen = 0;
  res = copyinstr(upathname, pathname, __PATH_MAX + 1, &pathnamelen);
  if (res) {
    goto SYS_CHDIR_ERROR;
  }

  res = vfs_chdir(pathname);

  if (res) {
    goto SYS_CHDIR_ERROR;
  }

  goto SYS_CHDIR_ERROR_FREE;

SYS_CHDIR_ERROR_FREE:
  KASSERT(res == 0);
  return 0;

SYS_CHDIR_ERROR:
  KASSERT(res != 0);
  return res;
}
