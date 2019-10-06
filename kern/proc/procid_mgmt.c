#include <types.h>

#include <current.h>
#include <limits.h>
#include <proc.h>
#include <procid_mgmt.h>
#include <spinlock.h>
#include <synch.h>
#include <thread.h>

static struct sysprocs sysprocs;

void sysprocs_init(struct proc* init_proc)
{
  KASSERT(init_proc != NULL);
  KASSERT(sysprocs.sp_procs == NULL);

  spinlock_init(&sysprocs.sp_lock);

  sysprocs.sp_procs =
      (struct proc**)kmalloc(sizeof(struct proc*) * NUM_PROCESSES_MAX);
  KASSERT(sysprocs.sp_procs);
  memset(sysprocs.sp_procs, 0x0, sizeof(struct proc*) * NUM_PROCESSES_MAX);

  sysprocs.next_pid = PID_MIN;
}

/*
 * Linearly scan the sysprocs table for a free entry.  For this project,
 * an O(n) method is fine.  Assign the provided proc to this pid entry.
 */
pid_t allocate_pid(struct proc* proc)
{
  pid_t new_pid = 0;

  spinlock_acquire(&sysprocs.sp_lock);

  for (int i = 0; i <= NUM_PROCESSES_MAX; i++) {
    if (sysprocs.sp_procs[i] == NULL) {
      sysprocs.sp_procs[i] = proc;
      new_pid = sysprocs.next_pid;
      sysprocs.next_pid++;
      break;
    }
  }

  spinlock_release(&sysprocs.sp_lock);

  // Returning 0 is not good--this indicates we've run out of numbers.
  return new_pid;
}

pid_t sys_getpid(void)
{
  struct proc* proc = curproc;

  return proc->p_pid;
}

void unassign_pid(pid_t pid)
{
  KASSERT(sysprocs.sp_procs[pid] != NULL);
  sysprocs.sp_procs[pid] = NULL;

  KASSERT(0 && "Not implemented: unassign_pid");
}

struct proc* get_proc_from_pid(pid_t pid)
{
  struct proc* rvalue = NULL;

  spinlock_acquire(&sysprocs.sp_lock);

  /* scan sysprocs table for the pid */
  for (int i = 0; i <= NUM_PROCESSES_MAX; i++) {
    if (sysprocs.sp_procs[i] == NULL)
      continue;

    if (sysprocs.sp_procs[i]->p_pid == pid) {
      rvalue = sysprocs.sp_procs[i];
      break;
    }
  }

  spinlock_release(&sysprocs.sp_lock);

  return rvalue;
}

bool is_pid_my_child(pid_t pid)
{
  struct proc* proc = curproc;
  struct thread_list* tl = proc->p_mychild_threads;

  if (tl == NULL) {
    return false;
  }

  spinlock_acquire(&sysprocs.sp_lock);

  while (tl != NULL) {
    if (tl->tl_pid == pid) {
      break;
    }

    tl = tl->tl_next;
  }

  spinlock_release(&sysprocs.sp_lock);

  if (tl)
    return true;

  return false;
}
