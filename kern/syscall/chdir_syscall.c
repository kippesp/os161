#include <kern/errno.h>

#include <types.h>

#include <copyinout.h>
#include <current.h>
#include <syscall.h>
#include <limits.h>
#include <proc.h>
#include <vfs.h>

int sys_chdir(const_userptr_t upathname)
{
  int res = 0;
  struct proc* proc = curproc;

  lock_acquire(proc->p_lk_syscall);

  char pathname[PATH_MAX];
  size_t pathnamelen;

  res = copyinstr(upathname, pathname, PATH_MAX, &pathnamelen);
  if (res) {
    goto SYS_CHDIR_ERROR;
  }

  res = vfs_chdir(pathname);

  if (res) {
    goto SYS_CHDIR_ERROR;
  }

  goto SYS_CHDIR_ERROR_FREE;

SYS_CHDIR_ERROR_FREE:
  lock_release(proc->p_lk_syscall);
  KASSERT(res == 0);
  return 0;

SYS_CHDIR_ERROR:
  lock_release(proc->p_lk_syscall);
  KASSERT(res != 0);
  return res;
}
