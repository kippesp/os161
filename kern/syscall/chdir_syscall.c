#include <kern/errno.h>

#include <types.h>

#include <copyinout.h>
#include <file_syscall.h>
#include <limits.h>
#include <vfs.h>

int sys_chdir(const_userptr_t upathname)
{
  int res = 0;

  if (upathname == NULL) {
    res = EFAULT;
    goto SYS_CHDIR_ERROR;
  }

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
