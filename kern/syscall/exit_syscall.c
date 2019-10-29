#include <kern/wait.h>

#include <types.h>

#include <current.h>
#include <proc.h>
#include <synch.h>
#include <syscall.h>

void sys__exit(int exitcode)
{
  int prepped_code = _MKWAIT_EXIT(exitcode);

  struct proc* proc = curproc;

  spinlock_acquire(&proc->p_lock);
  proc->p_exit_status = prepped_code;
  spinlock_release(&proc->p_lock);

  /* wake any process sleeping on thread exiting */

  lock_acquire(proc->p_lk_exited);
  proc->p_exited = true;
  cv_signal(proc->p_cv_exited, proc->p_lk_exited);
  lock_release(proc->p_lk_exited);

  /*
   * If the process forked any child threads that are still running,
   * orphan them.
   */

  lock_acquire(proc->p_lk_mychild_threads);
  unassociate_all_pids_from_parent();
  lock_release(proc->p_lk_mychild_threads);

  // Call does not return
  thread_exit();
}
