#include <kern/errno.h>
#include <types.h>

#include <current.h>
#include <filedescr.h>
#include <file_syscall.h>
#include <synch.h>

int sys_dup2(int oldfh, int newfh)
{
  (void)newfh;
  int res = 0;
  struct filedesc* oldfd;

  KASSERT(0 && "dup2() not implemented");

  res = get_fd(curproc, oldfh, &oldfd);

  if (res) {
    goto SYS_DUP2_ERROR;
  }

  struct filedesc* newfd = new_fd();

  if (newfd == NULL) {
    res = ENOMEM;
    goto SYS_DUP2_ERROR;
  }


  goto SYS_DUP2_ERROR_FREE;

SYS_DUP2_ERROR_FREE:
  KASSERT(res == 0);
  return 0;

SYS_DUP2_ERROR:
  KASSERT(res != 0);
  return res;
}
