#include <limits.h>
#include <proc.h>
#include <procid_mgmt.h>
#include <spinlock.h>
#include <synch.h>
#include <thread.h>

static struct sysprocs sysprocs;

pid_t sysprocs_init(struct proc* init_proc)
{
  KASSERT(init_proc != NULL);
  KASSERT(sysprocs.sp_procs == NULL);

  pid_t INIT_PID = 1;

  spinlock_init(&sysprocs.sp_lock);

  sysprocs.sp_procs =
      (struct proc**)kmalloc(sizeof(struct proc*) * NUM_PROCESSES_MAX);
  KASSERT(sysprocs.sp_procs);
  memset(sysprocs.sp_procs, 0x0, sizeof(struct proc*) * NUM_PROCESSES_MAX);

  /* pid 0 is special.  Never use it. */
  sysprocs.sp_procs[0] = (struct proc*)0xDEADBEEF;

  sysprocs.sp_procs[INIT_PID] = init_proc;

  KASSERT(PID_MIN > 1);
  sysprocs.next_pid = PID_MIN;

  return INIT_PID;
}

/*
 * Linearly scan the sysprocs table for a free entry.  For this project,
 * an O(n) method is fine.  Assign the provided proc to this pid entry.
 */
pid_t assign_pid(struct proc* proc)
{
  for (int i = PID_MIN; i <= NUM_PROCESSES_MAX; i++) {
    if (sysprocs.sp_procs[i] == NULL) {
      sysprocs.sp_procs[i] = proc;
      pid_t assigned_pid = sysprocs.next_pid;
      sysprocs.next_pid++;
      return assigned_pid;
    }
  }

  KASSERT(0 && "Ran out of process table entries");

  return -1;
}

void unassign_pid(pid_t pid)
{
  KASSERT(sysprocs.sp_procs[pid] != NULL);
  sysprocs.sp_procs[pid] = NULL;

  KASSERT(0 && "Not implemented");
}
