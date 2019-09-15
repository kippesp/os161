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
  for (int i = 0; i <= NUM_PROCESSES_MAX; i++) {
    if (sysprocs.sp_procs[i] == NULL) {
      sysprocs.sp_procs[i] = proc;
      pid_t assigned_pid = sysprocs.next_pid;
      sysprocs.next_pid++;
      return assigned_pid;
    }
  }

  // Returning 0 is not good--this indicates we've run out of numbers.
  return 0;
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

  KASSERT(0 && "Not implemented");
}

pid_t get_procfrompid(struct proc* proc)
{
  (void)proc;

  KASSERT(0 && "Not implemented");
}
