#ifndef _FILE_SYSCALL_H_
#define _FILE_SYSCALL_H_

#include <types.h>

/*
 * I use the convention
 *    fh - file handle       - index into the process file table
 *    fd - file descriptor   - a single file descriptor instance
 *    struct file descriptor - the structure definition of an open file
 */

#define IS_OFLAGS_RO(x) (((x) & (O_WRONLY | O_RDWR)) == 0x0)

int sys_open(const_userptr_t filename, int oflags, int* fh);
int sys_read(int fh, userptr_t ubuf, size_t buflen, ssize_t* buflen_read);
int sys_write(int fh, const_userptr_t ubuf, size_t buflen,
              ssize_t* buflen_written);
int sys_close(int fh);
off_t sys_lseek(int fh, off_t pos, int whence, off_t* new_pos);
int sys_chdir(const_userptr_t);
int sys___getcwd(userptr_t* buf, size_t buflen);
int sys_dup2(int oldfd, int newfd);

#endif /* _FILE_SYSCALL_H_ */
