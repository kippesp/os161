#include <types.h>

#include <addrspace.h>
#include <copyinout.h>
#include <current.h>
#include <kern/errno.h>
#include <kern/syscall.h>
#include <lib.h>
#include <mips/trapframe.h>
#include <proc.h>

#include <fork_syscall.h>
#include <procid_mgmt.h>

struct forked_child_init_data {
  struct trapframe* c_tf;
  struct proc* c_proc;
};

static void forked_child_entrypoint(void* data1, unsigned long data2)
{
  (void)data2;
  (void)data1;

  struct forked_child_init_data* init_data =
      (struct forked_child_init_data*)data1;

  /*
   * The forked child starts with a fresh stack (i.e. no exists calls)
   * and a trapframe based on the parent's.
   */
  init_data->c_tf->tf_a3 = 0;   /* return value */
  init_data->c_tf->tf_epc += 4; /* skip to next instruction */

  /* Activate the new thread's address space. */
  as_activate();

  /* Create a new copy of the trapframe on the kernel stack.  Provide its
   * pointer to mips_usermode. */

  struct trapframe tf;
  memcpy(&tf, init_data->c_tf, sizeof(struct trapframe));

  mips_usermode(&tf);
}

/*
 * Enter user mode for a newly forked process.
 *
 * This function is provided as a reminder. You need to write
 * both it and the code that calls it.
 *
 * Thus, you can trash it and do things another way if you prefer.
 */
void enter_forked_process(struct trapframe* tf)
{
  // TODO: Use me!
  (void)tf;
}

int sys_fork(const_userptr_t tf, pid_t* pid_or_zero)
{
  int res = 0;

  /*
   * Required data to the forked child's entrypoint function is passed via a
   * single structure.
   */
  struct forked_child_init_data* cinitd =
      kmalloc(sizeof(struct forked_child_init_data));

  if (cinitd == NULL) {
    res = ENOMEM;
    goto SYS_FORK_ERROR;
  }

  // ALLOCATION: cinitd

  /*
   * Duplicate the caller's trapframe (which was placed on the stack).  Before
   * entering the forked process, the trapframe will be copied onto the kernel
   * stack before passing control to the child via mips_usermode().
   */
  cinitd->c_tf = (struct trapframe*)kmalloc(sizeof(struct trapframe));

  if (cinitd->c_tf == NULL) {
    kfree(cinitd);
    res = ENOMEM;
    goto SYS_FORK_ERROR;
  }

  res = copyin(tf, cinitd->c_tf, sizeof(struct trapframe));
  KASSERT((res == 0) && "This should not have failed");

  // ALLOCATION: cinitd->c_tf
  // ALLOCATION: cinitd

  /*
   * ASST2 does not go beyond the one thread per process model.  Create a new
   * process and add the new child thread to it.
   */
  struct proc* proc = curproc;
  cinitd->c_proc = proc_create(proc->p_name);

  if (cinitd->c_proc == NULL) {
    res = ENOMEM;
    kfree(cinitd->c_tf);
    kfree(cinitd);
    goto SYS_FORK_ERROR;
  }

  // ALLOCATION: cinitd->c_proc
  // ALLOCATION: cinitd->c_tf
  // ALLOCATION: cinitd

  /*
   * Duplicate the caller's address space for the child.
   */
  res = as_copy(proc->p_addrspace, &cinitd->c_proc->p_addrspace);

  if (cinitd->c_proc->p_addrspace == NULL) {
    res = ENOMEM;
    proc_uncreate(cinitd->c_proc);
    kfree(cinitd->c_tf);
    kfree(cinitd);
    goto SYS_FORK_ERROR;
  }

  // ALLOCATION: cinitd->c_proc->p_addrspace
  // ALLOCATION: cinitd->c_proc
  // ALLOCATION: cinitd
  // ALLOCATION: cinitd->c_tf

  /*
   * Duplicate the caller's open files table and bump the refcount in the
   * descriptors.
   */
  cinitd->c_proc->p_fdtable = dup_fdtable(proc->p_fdtable);

  if (cinitd->c_proc->p_fdtable == NULL) {
    res = ENOMEM;
    as_destroy(cinitd->c_proc->p_addrspace);
    cinitd->c_proc->p_addrspace = NULL;
    proc_uncreate(cinitd->c_proc);
    kfree(cinitd->c_tf);
    kfree(cinitd);
    goto SYS_FORK_ERROR;
  }

  // ALLOCATION: cinitd->c_proc->p_fdtable
  // ALLOCATION: cinitd->c_proc->p_addrspace
  // ALLOCATION: cinitd->c_proc
  // ALLOCATION: cinitd
  // ALLOCATION: cinitd->c_tf

  /*
   * Give the new process its own pid and tell it who its parent is.
   */
  cinitd->c_proc->p_pid = allocate_pid(cinitd->c_proc);

  /* A pid==0 here indicates there are no more pids in the sysprocs table */
  if (cinitd->c_proc->p_pid == 0) {
    res = ENPROC;
    undup_fdtable(cinitd->c_proc->p_fdtable);
    cinitd->c_proc->p_fdtable = NULL;
    as_destroy(cinitd->c_proc->p_addrspace);
    cinitd->c_proc->p_addrspace = NULL;
    proc_uncreate(cinitd->c_proc);
    kfree(cinitd->c_tf);
    kfree(cinitd);
    goto SYS_FORK_ERROR;
  }

  cinitd->c_proc->p_ppid = proc->p_pid;

  /*
   * Call the entrypoint for the forked child
   */

  pid_t ppid = proc->p_pid;
  pid_t cpid = cinitd->c_proc->p_pid;
  res = thread_fork(proc->p_name, cinitd->c_proc, forked_child_entrypoint,
                    (void*)&cinitd, 0);
  // TODO: Race Condition!  When can I free this?  A lock in the struct?
  // kfree(cinitd);

  KASSERT(res && "Didn't work");

  if (ppid == proc->p_pid) {
    *pid_or_zero = 0;
  }
  else {
    *pid_or_zero = cpid;
  }

  kfree(cinitd->c_tf);
  kfree(cinitd);

  goto SYS_FORK_ERROR_FREE;

  // TODO: Once working, clean up this function

SYS_FORK_ERROR_FREE:
  KASSERT(res == 0);
  return 0;

SYS_FORK_ERROR:
  KASSERT(res != 0);
  return res;
}
