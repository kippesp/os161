#ifndef _PROCUD_MGMT_H_
#define _PROCUD_MGMT_H_

#include <types.h>

#include <limits.h>
#include <spinlock.h>

struct proc;

/* system process ID list */
struct sysprocs {
  int i_am_initialized;
  struct spinlock sp_lock;
  struct proc* sp_procs[NUM_PROCESSES_MAX];
  pid_t next_pid;
};

/* Allocate and initialize the system processes table */
void sysprocs_init(struct proc*);

/* Allocate a new entry in the sysprocs table; return its pid */
pid_t allocate_pid(struct proc*);

/* Deallocate pid's entry in the sysprocs table */
void unassign_pid(pid_t pid);

/* Return the pid of the current process */
pid_t sys_getpid(void);

/* Return the pid of the provided process */
struct proc* get_proc_from_pid(pid_t pid);

/* Return true if pid is child of the current process */
bool is_pid_my_child(pid_t pid);
#endif
