#ifndef _FILE_SYSCALL_H_
#define _FILE_SYSCALL_H_

#include <types.h>

int sys_write(int, const_userptr_t, size_t, ssize_t*);

#endif /* _FILE_SYSCALL_H_ */
