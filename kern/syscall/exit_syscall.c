#include <kern/wait.h>

#include <types.h>

#include <current.h>
#include <proc.h>
#include <synch.h>

#include <exit_syscall.h>

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

  // Call does not return
  thread_exit();
}
