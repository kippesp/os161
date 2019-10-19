/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Sample/test code for running a user program.  You can use this for
 * reference when implementing the execv() system call. Remember though
 * that execv() needs to do more than runprogram() does.
 */

#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <lib.h>
#include <proc.h>
#include <current.h>
#include <addrspace.h>
#include <vm.h>
#include <vfs.h>
#include <syscall.h>
#include <test.h>

#include <align.h>
#include <filedescr.h>
#include <kern/unistd.h>
#include <synch.h>
#include <vnode.h>
#include <procid_mgmt.h>

/* Helper function to create stdin/out/err */
static struct filedesc* open_console_device(int OFLAGS, const char* name)
{
  char fn[80];
  struct vnode* fd_ofile = NULL;
  int open_res;
  struct filedesc* fd = NULL;

  strcpy(fn, "con:");

  open_res = vfs_open(fn, OFLAGS, 0666, &fd_ofile);
  KASSERT((open_res == 0) && "Error opening con: device");

  /* vn_fs being NULL is used to determine if syscalls are to mess with things
     like offsets and positioning. */
  KASSERT((fd_ofile->vn_fs == NULL) && "vn_fs isn't null for con: device");

  KASSERT(strlen(name) < 75);

  char lk_name[80];
  strcpy(lk_name, "fd_");
  strcat(lk_name, name);

  /* Create the file descriptor */
  fd = kmalloc(sizeof(struct filedesc));
  KASSERT(fd);
  memset(fd, 0x0, sizeof(struct filedesc));
  fd->fd_refcnt = 1;
  fd->fd_ofile = fd_ofile;
  fd->oflags = OFLAGS;
  fd->fd_lock = lock_create(lk_name);

  return fd;
}

/*
 * Load program "progname" and start running it in usermode.
 * Does not return except on error.
 *
 * Calls vfs_open on progname and thus may destroy it.
 */
int runprogram(char* progname, int argc, char** args)
{
  struct addrspace* as;
  struct vnode* v;
  vaddr_t entrypoint, stackptr;
  int result;

  /* Open the file. */
  result = vfs_open(progname, O_RDONLY, 0, &v);
  if (result) {
    return result;
  }

  /* We should be a new process. */
  KASSERT(proc_getas() == NULL);

  /* Create a new address space. */
  as = as_create();
  if (as == NULL) {
    vfs_close(v);
    return ENOMEM;
  }

  /* Switch to it and activate it. */
  proc_setas(as);
  as_activate();

  /* Load the executable. */
  result = load_elf(v, &entrypoint);
  if (result) {
    /* p_addrspace will go away when curproc is destroyed */
    vfs_close(v);
    return result;
  }

  /* Done with the file now. */
  vfs_close(v);

  /* Define the user stack in the address space */
  result = as_define_stack(as, &stackptr);
  if (result) {
    /* p_addrspace will go away when curproc is destroyed */
    return result;
  }

  /* Create an empty file handles table */
  struct filedesc** p_fdtable = init_fdtable();

  if (p_fdtable == NULL) {
    return ENOMEM;
  }

  struct filedesc* fd = NULL;
  struct proc* proc = curproc;

  proc->p_fdtable = p_fdtable;

  /* open stdout */
  fd = open_console_device(O_WRONLY, "stdout");
  KASSERT(proc->p_fdtable[STDOUT_FILENO] == NULL);
  proc->p_fdtable[STDOUT_FILENO] = fd;

  /* open stderr */
  fd = open_console_device(O_WRONLY, "stderr");
  KASSERT(proc->p_fdtable[STDERR_FILENO] == NULL);
  proc->p_fdtable[STDERR_FILENO] = fd;

  /* open stdin */
  fd = open_console_device(O_RDONLY, "stdin");
  KASSERT(proc->p_fdtable[STDIN_FILENO] == NULL);
  proc->p_fdtable[STDIN_FILENO] = fd;

  /* initalize the system process identifier table */
  sysprocs_init(proc);
  pid_t pid = assign_pid(proc);
  proc->p_pid = pid;

  // TODO: set cwd to "/"

  /*
   * Construct argv array structure for the new userspace stack.  The
   * userspace stack pointer is defined to be correctly aligned and starts as
   * USERSTACK, growing towards 0.  The data is layed out by the helper function
   * into the correct locations of the userspace pointer.
   *
   * All data is aligned on int boundaries.
   *
   * TOP-OF-USER-STACK / USERSTACK / 0x80000000
   * char*     argv[0]          typically the program name being executed
   * char*     argv[1]          string of the first argument
   * char*     argv[2..argc-1]
   * userptr_t argv[argc]       always NULL
   * userptr_t argv[2..argc-1]
   * userptr_t argv[1]          absolute pointer to &argv[1]
   * userptr_t argv[0]          absolute pointer to &argv[0]
   * BASE-OF-USER-STACK <-- stackptr when above is set up
   *
   * Each string argv[0]..argv[argc-1] is stored packed and is aligned on int
   * boundaries.  The alignment is enforced by postpended the strings with
   * '\0' as necessary.
   *
   * First the amount of data the argv array structure consumes is determined.
   * The entire area is initialized.  Then it is populated.
   *
   * If the structure is larger than __ARG_MAX bytes, then this is an error.
   */

  userptr_t argv;

  {
    size_t bytes_for_argv_strings = 0;

    /* calculate bytes to store args */
    for (int i = 0; i < argc; i++) {
      /*
       * All args[n] will be copied while padding out the string with '\0' to
       * maintain the int alignment.  We could calculate the address here,
       * but we won't.
       */
      bytes_for_argv_strings +=
          ALIGNED_SIZE_T(sizeof(char*), strlen(args[i]) + 1);
    }

    /* check size won't go over the limit */
    size_t total_argv_size = bytes_for_argv_strings + (argc + 1) * sizeof(char*);
    KASSERT(total_argv_size <= __ARG_MAX);

    /*
     * Use argvptr to set the addresses of the argv strings--brilliant!
     * Skip over bytes_for_argv_strings and set the NULL terminator pointer
     * of the argv list.
     */
    vaddr_t argvptr = stackptr - bytes_for_argv_strings - sizeof(char*);
    *(vaddr_t*)argvptr = 0x0; /* this is NULL terminator at argv[argc] */

    /* initialize the area where the strings will be stored */
    memset((char*)(argvptr + sizeof(char*)), 0x0, bytes_for_argv_strings);

    /*
     * Initial conditions: argsptr (the string storage addresses) will start at
     * its first location (just above argv[argc]--the NULL terminator). argvptr
     * (the pointer to the strings) will start at the NULL terminator. From
     * there, the two pointers will move in opposite directions--argsptr up and
     * argvptr down.
     */

    vaddr_t argsptr = argvptr + sizeof(char*);

    for (int i = argc - 1; i >= 0; i--) {
      /* move argvptr down */
      argvptr = argvptr - sizeof(char*);

      /* copy the string to address argsptr and store its the addr at argvptr */
      strcpy((char*)argsptr, args[i]);
      *(vaddr_t*)argvptr = argsptr;

      argsptr = argsptr + ALIGNED_SIZE_T(sizeof(char*), strlen(args[i]) + 1);
    }

    argv = (userptr_t)argvptr;

    /* Update the new stack pointer location */
    stackptr = (vaddr_t)argv;
  }

  /* Warp to user mode. */
  enter_new_process(argc, argv, NULL /*userspace addr of environment*/,
                    stackptr, entrypoint);

  /* enter_new_process does not return. */
  panic("enter_new_process returned\n");
  return EINVAL;
}
