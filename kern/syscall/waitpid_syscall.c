#include <kern/errno.h>
#include <kern/wait.h>

#include <types.h>

#include <copyinout.h>
#include <current.h>
#include <proc.h>
#include <synch.h>

#include <align.h>
#include <procid_mgmt.h>
#include <waitpid_syscall.h>

int sys_waitpid(pid_t tgt_pid, userptr_t tgt_status, int options, pid_t* rvalue)
{
  int res = 0;

  /* track return status internally since status is permitted to be NULL */
  int rstatus = 0;

  if (!IS_ALIGNED_USERPTR_T(tgt_status)) {
    res = EFAULT;
    goto SYS_WAITPID_ERROR;
  }

  /* short-circuit check that status pointer is valid or NULL */
  if (tgt_status != NULL) {
    if (copyin(tgt_status, &rstatus, sizeof(int))) {
      res = EFAULT;
      goto SYS_WAITPID_ERROR;
    }
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
  lock_acquire(tgt_proc->p_lk_exited);
  while (!tgt_proc->p_exited) {
    cv_wait(tgt_proc->p_cv_exited, tgt_proc->p_lk_exited);
  }
  lock_release(tgt_proc->p_lk_exited);

  /*
   * At this point, the child is on its way to exiting.  Yield until the number
   * of threads to be 0 which will be decremented by proc_remthread().
   */

  bool all_threads_exited = false;
  do {
    spinlock_acquire(&tgt_proc->p_lock);

    if (tgt_proc->p_numthreads == 0) {
      all_threads_exited = true;
    } else {
      /* During testing thread_yield() was never observed to have occurred. */
      thread_yield();
    }

    spinlock_release(&tgt_proc->p_lock);
  } while (all_threads_exited == false);

  /*
   * The child has effectively exited, but the process structure still exists.
   * The parent can freely manipulate the child's process structure and do
   * whatever to clean up the terminated process then delete it.
   */
  rstatus = tgt_proc->p_exit_status;

  /* Remove the (now mostly dead) child pid from the parent */
  struct proc* proc = curproc;
  lock_acquire(proc->p_lk_mychild_threads);
  unassociate_child_pid_from_parent(proc, tgt_pid);

  /*
   * If the zombie child forked its own children, reparent these threads to
   * themselves (i.e. set ppid := pid).  OS161 has no init process, so there
   * is little alternative.
   */
  KASSERT((tgt_proc->p_mychild_threads == NULL) && "This needs testing.");

  pid_t orphaned_pid;
  struct thread_list* orphaned_thread_list = tgt_proc->p_mychild_threads;
  while (orphaned_thread_list != NULL) {
    KASSERT(orphaned_thread_list != NULL);
    orphaned_pid = orphaned_thread_list->tl_pid;
    struct proc* orphaned_proc = get_proc_from_pid(orphaned_pid);

    KASSERT(orphaned_proc->p_ppid == tgt_pid);
    KASSERT(orphaned_proc->p_parent_proc == tgt_proc);

    /* Self parent the orphan */
    orphaned_proc->p_ppid = orphaned_proc->p_pid;
    orphaned_proc->p_parent_proc = NULL; /* NULL probably better than self */

    lock_acquire(tgt_proc->p_lk_mychild_threads);
    unassociate_child_pid_from_parent(tgt_proc, orphaned_pid);
    lock_release(tgt_proc->p_lk_mychild_threads);
  }

  lock_release(proc->p_lk_mychild_threads);

  /* Orphan this thread */
  tgt_proc->p_ppid = tgt_proc->p_pid;
  tgt_proc->p_parent_proc = NULL;

  proc_destroy(tgt_proc);

  if (tgt_status != NULL) {
    if (copyout(&rstatus, tgt_status, sizeof(int))) {
      res = EFAULT;
      goto SYS_WAITPID_ERROR;
    }
  }

  KASSERT(rvalue != NULL);
  *rvalue = tgt_pid;

  goto SYS_WAITPID_ERROR_FREE;

SYS_WAITPID_ERROR_FREE:
  KASSERT(res == 0);
  return 0;

SYS_WAITPID_ERROR:
  KASSERT(res != 0);
  return res;
}
