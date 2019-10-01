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

  lock_acquire(proc->p_lk_exited);
  proc->p_exited = true;
  cv_signal(proc->p_cv_exited, proc->p_lk_exited);
  lock_release(proc->p_lk_exited);

  // The thread's work is done.  Release the lock and kill itself.

  // TODO: check if parent still exists
  // TODO: if not, get me a new parent

  // TODO: if I am my own parent, I can destroy myself.

  // Call does not return
  thread_exit();
}
