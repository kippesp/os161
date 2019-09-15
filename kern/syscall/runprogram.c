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
int runprogram(char* progname)
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

  pid_t pid = allocate_pid(proc);
  proc->p_pid = pid;

  /* Warp to user mode. */
  enter_new_process(0 /*argc*/, NULL /*userspace addr of argv*/,
                    NULL /*userspace addr of environment*/, stackptr,
                    entrypoint);

  /* enter_new_process does not return. */
  panic("enter_new_process returned\n");
  return EINVAL;
}
