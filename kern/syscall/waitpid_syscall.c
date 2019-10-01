#include <kern/errno.h>
#include <kern/wait.h>

#include <types.h>

#include <copyinout.h>
#include <lib.h>

#include <procid_mgmt.h>
#include <waitpid_syscall.h>

int sys_waitpid(pid_t tgt_pid, userptr_t tgt_status, int options, pid_t* rvalue)
{
  (void)rvalue;

  int res = 0;

  /* track return status internally since status is permitted to be NULL */
  int rstatus = 0;

  /* short-circuit check that status pointer is valid */
  if (copyin(tgt_status, &rstatus, sizeof(int))) {
    res = EFAULT;
    goto SYS_WAITPID_ERROR;
  }

  /* check for invalid options */
  int supported_options = WNOHANG;
  if (options & ~supported_options) {
    res = EINVAL;
    goto SYS_WAITPID_ERROR;
  }

  /* check pid is known */

  struct proc* tgt_proc = get_proc_from_pid(tgt_pid);

  if (tgt_proc == NULL) {
    res = ESRCH;
    goto SYS_WAITPID_ERROR;
  }

  /* check pid is child of current process */

  if (!is_pid_my_child(tgt_pid)) {
    res = ECHILD;
    goto SYS_WAITPID_ERROR;
  }

  // TODO: for WNOHANG options; if child hasn't exited, return now
  // TODO: lock p_exited_lock
  // TODO: check p_exited

  // See userland/testbin/badcall/bad_waitpid.c
  // wait_badpid

  // Check pid exists
  // Check I am that pid's parent


  // TODO: Consider WNOHANG options (return fast if not yet exited)
  // TODO: This is supported by ./userland/bin/sh/sh.c


  /*
  So basically, we need to check:

    Is the status pointer properly aligned (by 4) ?
    Is the status pointer a valid pointer anyway (NULL, point to kernel, ...)?
    Is options valid? (More flags than WNOHANG | WUNTRACED )
    Does the waited pid exist/valid?
    If exist, are we allowed to wait it ? (Is it our child?)

  And also, after successfully get the exitcode, don't forget to destroy the
  child's process structure and free its slot in the procs array. Since one
  child has only one parent, and after we wait for it, no one will care for it
  any more!
  */

  (void)rstatus;

  goto SYS_WAITPID_ERROR_FREE;

SYS_WAITPID_ERROR_FREE:
  KASSERT(res == 0);
  return 0;

SYS_WAITPID_ERROR:
  KASSERT(res != 0);
  return res;
}
