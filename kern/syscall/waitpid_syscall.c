#include <kern/errno.h>
#include <kern/wait.h>

#include <types.h>

#include <copyinout.h>
#include <current.h>
#include <proc.h>
#include <synch.h>

#include <procid_mgmt.h>
#include <waitpid_syscall.h>

int sys_waitpid(pid_t tgt_pid, userptr_t tgt_status, int options, pid_t* rvalue)
{
  (void)rvalue;

  int res = 0;

  /* track return status internally since status is permitted to be NULL */
  int rstatus = 0;

  /* short-circuit check that status pointer is valid */

#if 0
  /* TODO: check alignment -- Where is the requirement? */
  KASSERT(sizeof(unsigned int) >= sizeof(userptr_t));
  unsigned int align_mask = ~(unsigned int)sizeof(int);
  if ((unsigned int)tgt_status != ((unsigned int)tgt_status & align_mask)) {
    kprintf("*** PAUL: status address is not aligned\n");
  }
#endif

  if (copyin(tgt_status, &rstatus, sizeof(int))) {
    res = EFAULT;
    goto SYS_WAITPID_ERROR;
  }

  /* check for invalid options */

#if 0
  int supported_options = WNOHANG;
#else
  int supported_options = 0;
#endif

  if (options & ~supported_options) {
    res = EINVAL;
    goto SYS_WAITPID_ERROR;
  }

  /* check pid exists/is known */

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

#if 0
  /*
   * If WNOHANG option is specified, check if child has exited.  If not,
   * return now with 0.
   */
  if (options & WNOHANG) {
    spinlock_acquire(&tgt_proc->p_exited_lock);
    bool has_exited = tgt_proc->p_exited;
    spinlock_release(&tgt_proc->p_exited_lock);

    if (!has_exited)
      return 0;
  }
#endif

  /* sleep until the child exits */

  // TODO: restructure to use the thread_count wait channel

  lock_acquire(tgt_proc->p_lk_exited);
  while (!tgt_proc->p_exited) {
    cv_wait(tgt_proc->p_cv_exited, tgt_proc->p_lk_exited);
  }
  lock_release(tgt_proc->p_lk_exited);

  /*
   * At this point, the child is on its way to exiting.  Wait for the
   * number of threads to be 0.
   */

#if 0
  bool all_threads_exited = false;
  do {
    spinlock_acquire(&tgt_proc->p_lock);

    if (tgt_proc->p_numthreads == 0) {
      all_threads_exited = true;
    } else {
      // TODO: create a wait channel
    }

    spinlock_release(&tgt_proc->p_lock);
  } while (all_threads_exited == false);
#endif

  /*
   * At this point, the child has effectively exited, but the process structure
   * still exists.  The parent can freely manipulate the child's process
   * structure and do whatever to clean up the terminated process then delete
   * it.
   */

  rstatus = tgt_proc->p_exit_status;

  /* disassociate the child proc from the parent */

  // TODO: struct proc* proc = curproc;

  unassociate_child_pid_from_parent(tgt_pid);

  /*
   * If the zombie child forked its own children, reparent these threads to
   * themselves (i.e. set ppid := pid).
   */
  KASSERT(tgt_proc->p_mychild_threads == NULL);  // TODO

  // TODO: set return parm status if not null
  // TODO: deallocate all the stuff in the child process struct
  // TODO: what to do if the child forked its own children?
  // TODO:    should I become the parent of these threads?

  // TODO: perhaps create a test x86 app for my os161 locks; then test out
  //       some of these issues with fork of forked children.

  // See userland/testbin/badcall/bad_waitpid.c
  // wait_badpid

  proc_destroy(tgt_proc);
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

  if (copyout(&rstatus, tgt_status, sizeof(int))) {
    res = EFAULT;
    goto SYS_WAITPID_ERROR;
  }

  goto SYS_WAITPID_ERROR_FREE;

SYS_WAITPID_ERROR_FREE:
  KASSERT(res == 0);
  return 0;

SYS_WAITPID_ERROR:
  KASSERT(res != 0);
  return res;
}
