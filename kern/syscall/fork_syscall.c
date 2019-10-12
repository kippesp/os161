#include <types.h>

#include <addrspace.h>
#include <copyinout.h>
#include <current.h>
#include <kern/errno.h>
#include <kern/syscall.h>
#include <lib.h>
#include <mips/trapframe.h>
#include <proc.h>
#include <synch.h>
#include <vfs.h>

#include <fork_syscall.h>
#include <procid_mgmt.h>

struct forked_child_init_data {
  struct trapframe* c_tf;
  struct proc* c_proc;
  struct semaphore* waitforchildstart;
};

/*
 * Enter user mode for a newly forked process.
 *
 * This function is provided as a reminder. You need to write
 * both it and the code that calls it.
 *
 * Thus, you can trash it and do things another way if you prefer.
 */
static void enter_forked_process(void* data1, unsigned long data2)
{
  (void)data2;

  struct forked_child_init_data* init_data =
      (struct forked_child_init_data*)data1;

  /*
   * The forked child starts with a fresh stack (i.e. no exists calls)
   * and a trapframe based on the parent's.
   */
  init_data->c_tf->tf_a3 = 0;   /* return value indicating a child thread */
  init_data->c_tf->tf_epc += 4; /* skip to next instruction */

  /* Activate the new thread's address space. */
  as_activate();

  /*
   * Create a new copy of the trapframe on the kernel stack.  Provide its
   * pointer to mips_usermode.
   */

  struct trapframe tf;
  memcpy(&tf, init_data->c_tf, sizeof(struct trapframe));

  // kprintf("entering child....\n");

#if 0
  /* TODO */
  char c[256];
  strcpy(c, "/");
  int res = vfs_chdir(c);
  KASSERT(res == 0);
#endif

  /* Signal parent its okay to deallocate the passed trapframe */
  V(init_data->waitforchildstart);

  /* Return back to user mode in the forked child */
  mips_usermode(&tf);
}

int sys_fork(const_userptr_t tf, pid_t* child_pid)
{
  int res = 0;

  /*
   * Data for the forked child's entrypoint function is passed via a single
   * structure.
   */
  struct forked_child_init_data* cinitd =
      kmalloc(sizeof(struct forked_child_init_data));

  if (cinitd == NULL) {
    res = ENOMEM;
    goto SYS_FORK_ERROR_A;
  }

  /*
   * Duplicate the caller's trapframe (which was placed on the stack).  Before
   * entering the forked process, the trapframe will be copied onto the kernel
   * stack before passing control to the child via mips_usermode().
   */
  cinitd->c_tf = (struct trapframe*)kmalloc(sizeof(struct trapframe));

  if (cinitd->c_tf == NULL) {
    res = ENOMEM;
    goto SYS_FORK_ERROR_B;
  }

  memcpy(cinitd->c_tf, tf, sizeof(struct trapframe));

  /*
   * ASST2 does not go beyond the one thread per process model.  Create a new
   * process and add the new child thread to it.
   */
  struct proc* proc = curproc;
  cinitd->c_proc = proc_create(proc->p_name);

  if (cinitd->c_proc == NULL) {
    res = ENOMEM;
    goto SYS_FORK_ERROR_C;
  }

  /*
   * Duplicate the caller's address space for the child.
   */
  res = as_copy(proc->p_addrspace, &cinitd->c_proc->p_addrspace);

  if (cinitd->c_proc->p_addrspace == NULL) {
    res = ENOMEM;
    goto SYS_FORK_ERROR_D;
  }

  /*
   * Duplicate the caller's open files table and bump the refcounts of the
   * descriptors.
   */
  cinitd->c_proc->p_fdtable = dup_fdtable(proc->p_fdtable);

  if (cinitd->c_proc->p_fdtable == NULL) {
    res = ENOMEM;
    goto SYS_FORK_ERROR_E;
  }

  /*
   * Give the new process its own pid and tell it who its parent is.  Record
   * both the parent's ID and proc address in the rare chance the pids roll;
   * used as a double check to see if parent has died.
   */
  cinitd->c_proc->p_pid = allocate_pid(cinitd->c_proc);
  cinitd->c_proc->p_parent_proc = proc;
  cinitd->c_proc->p_ppid = proc->p_pid;

  /*
   * A p_pid==0 from allocate_pid() indicates there are no more pids in the
   * sysprocs table.
   */
  if (cinitd->c_proc->p_pid == 0) {
    res = ENPROC;
    goto SYS_FORK_ERROR_F;
  }

  /* Record the new pid in the parent to support waitpid. */
  res = associate_child_pid_in_parent(cinitd->c_proc->p_pid);

  if (res) {
    goto SYS_FORK_ERROR_G;
  }

  cinitd->waitforchildstart = sem_create("waitchildstart", 0);

  if (cinitd->waitforchildstart == NULL) {
    res = ENOMEM;
    goto SYS_FORK_ERROR_H;
  }

  /* Call the entrypoint for the forked child */
  pid_t cpid = cinitd->c_proc->p_pid;
  res = thread_fork(proc->p_name, cinitd->c_proc, enter_forked_process,
                    (void*)cinitd, 0);

  if (res != 0) {
    goto SYS_FORK_ERROR_I;
  }

  /* Wait for child thread to be finished with the trapframe data */
  P(cinitd->waitforchildstart);

  sem_destroy(cinitd->waitforchildstart);
  kfree(cinitd->c_tf);
  kfree(cinitd);

  *child_pid = cpid;

  goto SYS_FORK_ERROR_FREE;

SYS_FORK_ERROR_FREE:
  KASSERT(res == 0);
  return 0;

SYS_FORK_ERROR_I:
  sem_destroy(cinitd->waitforchildstart);
SYS_FORK_ERROR_H:
  unassociate_child_pid_from_parent(proc, cinitd->c_proc->p_pid);
SYS_FORK_ERROR_G:
  unassign_pid(cinitd->c_proc->p_pid);
SYS_FORK_ERROR_F:
  undup_fdtable(cinitd->c_proc->p_fdtable);
SYS_FORK_ERROR_E:
  as_destroy(cinitd->c_proc->p_addrspace);
SYS_FORK_ERROR_D:
  proc_uncreate(cinitd->c_proc);
SYS_FORK_ERROR_C:
  kfree(cinitd->c_tf);
SYS_FORK_ERROR_B:
  kfree(cinitd);
SYS_FORK_ERROR_A:
  KASSERT(res != 0);
  return res;
}
