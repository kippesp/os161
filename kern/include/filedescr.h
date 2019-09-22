#ifndef _FILEDESCR_H_
#define _FILEDESCR_H_

#include <types.h>

struct proc;
struct vnode;
struct lock;

struct filedesc {
  int fd_refcnt;          /* reference count */
  struct vnode* fd_ofile; /* open file */
  int oflags;             /* open file flags */
  off_t fd_pos;           /* file read/write position */
  struct lock* fd_lock;   /* synchronization lock */
};


/*
 * Allocate and initialize a new file descriptor table.
 */
struct filedesc** init_fdtable(void);

/*
 * Return the file descriptor, fd, given a process's file handle, fh.
 */
int get_fd(struct proc*, int, struct filedesc**);

/*
 * Allocates and initializes a new file descriptor.
 */
struct filedesc* new_fd(void);

/*
 * Called once the file descriptor is ready to be deleted.  A lock must
 * be held on the file descriptor by the current process.  The refcount
 * must be 0.
 */
void destroy_fd(struct filedesc*);

/*
 * Allocates and initialies a new file descriptor at file handle, fh.  Returns
 * an appropriate error code on error and 0 on success.
 */
int new_fh(struct proc*, int* fh);

/*
 * Returns the file handle of the found file descriptor.  Returns -1 if not
 * found.
 */
int get_fh(struct proc*, struct filedesc*);

#endif /* _FILEDESCR_H_ */
