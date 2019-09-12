#ifndef _PROCUD_MGMT_H_
#define _PROCUD_MGMT_H_

#include <types.h>
#include <spinlock.h>

struct proc;

struct sysprocs {
  struct spinlock sp_lock;
  struct proc** sp_procs;
  pid_t next_pid;
};

/* Allocate and initialize the system processes table */
pid_t sysprocs_init(struct proc*);

/* Allocate a new entry in the sysprocs table; return its pid */
pid_t assign_pid(struct proc*);

/* Deallocate pid's entry in the sysprocs table */
void unassign_pid(pid_t pid);

#endif
