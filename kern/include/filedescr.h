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
 * Returns an index into the process file table `p_fh` to facilitate allocation
 * of a new file descriptor.  Returns -1 if not found.
 */
int get_free_fh(struct proc*);

/*
 * Returns the file handle of the found file descriptor.  Returns -1 if not
 * found.
 */
int get_fh(struct proc*, struct filedesc*);

/*
 * Makes available the process's file handle.  Returns -1 if the index is
 * invalid or not currently assigned.
 */
int free_fh(struct proc*, int);

#endif /* _FILEDESCR_H_ */
