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

#ifndef _SYSCALL_H_
#define _SYSCALL_H_


#include <cdefs.h> /* for __DEAD */
struct trapframe; /* from <machine/trapframe.h> */

/*
 * The system call dispatcher.
 */

void syscall(struct trapframe *tf);

/*
 * Support functions.
 */

/* Enter user mode. Does not return. */
__DEAD void enter_new_process(int argc, userptr_t argv, userptr_t env,
		       vaddr_t stackptr, vaddr_t entrypoint);


/*
 * Prototypes for IN-KERNEL entry points for system call implementations.
 */

int sys_reboot(int code);
int sys___time(userptr_t user_seconds, userptr_t user_nanoseconds);

/* ASST2.1 additions */

/*
 * Developer Note: I use the convention
 *    fh - file handle       - index into the process file table
 *    fd - file descriptor   - a single file descriptor instance
 *    struct file descriptor - the structure definition of an open file
 */

int sys_write(int fh, const_userptr_t ubuf, size_t buflen,
              ssize_t* buflen_written);

/* ASST2.2a additions */

#define IS_OFLAGS_RO(x) (((x) & (O_WRONLY | O_RDWR)) == 0x0)

int sys_open(const_userptr_t filename, int oflags, int* fh);
int sys_read(int fh, userptr_t ubuf, size_t buflen, ssize_t* buflen_read);
int sys_close(int fh);
off_t sys_lseek(int fh, off_t pos, int whence, off_t* new_pos);
int sys_chdir(const_userptr_t);
int sys___getcwd(userptr_t* buf, size_t buflen);
int sys_dup2(int oldfd, int newfd);

/* ASST2.2b additions */

void sys__exit(int exitcode);
int sys_execv(userptr_t program, userptr_t args);
int sys_fork(const_userptr_t tf, pid_t*);
int sys_waitpid(pid_t pid, userptr_t status, int options, pid_t*);

#endif /* _SYSCALL_H_ */
