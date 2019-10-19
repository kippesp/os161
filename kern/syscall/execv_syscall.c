#include <types.h>

#include <addrspace.h>
#include <copyinout.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <lib.h>
#include <limits.h>
#include <proc.h>
#include <syscall.h>
#include <vfs.h>

#include <align.h>

int sys_execv(userptr_t program, userptr_t args)
{
  int res = 0;
  char* argv_buf = NULL;

  /* capture the program name */
  char programname[512];
  memset(programname, 0x0, sizeof(programname));
  res = copyinstr(program, programname, sizeof(programname) - 1, NULL);

  /*
   * Copy the arguments into kernel space to check out any issues.
   * Since we don't know the size of the buffer we need to create, there
   * is little else we can do other than scan all the strings to check
   * the total length.  If at any point we exceed ARG_MAX we can error out.
   */

  size_t bytes_for_aligned_argv_strings = 0;
  int argc = 0;
  int argc_limit = 4096; /* 4K of arguments seems a generous limit */

  /* initialize uargs_ptr to the (possibly NULL) first args pointer */
  userptr_t* uargs_ptr = (userptr_t*)args;

  while ((*uargs_ptr != NULL) && (argc < argc_limit)) {
    int len = 0;

    /* scan the string to determine its int-aligned length */
    do {
      /* reached '\0' at end of string? */
      if ((*(char**)uargs_ptr)[len] == '\0') {
        bytes_for_aligned_argv_strings += ALIGNED_SIZE_T(sizeof(int), len + 1);

        break;
      }
      else {
        len++;
      }
    } while (bytes_for_aligned_argv_strings + len <= ARG_MAX);

    argc++;
    uargs_ptr++;
  }

  if ((argc > argc_limit) || (bytes_for_aligned_argv_strings > ARG_MAX)) {
    res = E2BIG;
    goto SYS_EXEC_ERROR;
  }

  /* check size won't go over the limit defined for this implementation */
  size_t total_argv_size =
      bytes_for_aligned_argv_strings + (argc + 1) * sizeof(char*);
  if (total_argv_size > ARG_MAX) {
    res = E2BIG;
    goto SYS_EXEC_ERROR;
  }

  /*
   * Allocate a structure in kernel space in order to build the argv structure.
   * Since the stack pointer isn't defined until later in the function, some
   * patchup will be done after the data has been copied to userspace.  To be
   * valid, each offsets (see below) will be updated with absolute addresses to
   * its argv string.  See runprogram() for the layout definition of arv once
   * it is copied to userspace.  The layout while in kernelspace is here:
   *
   * START-OF-argv_buf
   * int       argv_off[0]      offset to &argv[0]
   * int       argv_off[1]      offset to &argv[1]
   * int       argv_off[2..argc-1]
   * int       argv_off[argc]   always NULL
   * char*     argv[2..argc-1]
   * char*     argv[1]          string of the first argument
   * char*     argv[0]          typically the program name being executed
   * END-OF-argv_buf
   *
   * The layout appropriate for userspace is the same as described in
   * runprogram().
   */

  KASSERT(sizeof(int) == sizeof(userptr_t));

  argv_buf = (char*)kmalloc(total_argv_size);
  memset(argv_buf, 0x0, total_argv_size);

  /* pointer to the string; initially argv[5] */
  char* args_ptr = argv_buf + (argc + 1) * sizeof(char*);

  /* upper bound of string space */
  size_t argv_string_buf_space_remaining = bytes_for_aligned_argv_strings;

  /* offset to args; later will be patched with the absolute pointer */
  ssize_t offset_to_args_string = 2 * sizeof(char*);

  /* reset uargs_ptr to the first userspace args pointer */
  uargs_ptr = (userptr_t*)args;

  for (int i = argc - 1; i >= 0; i--) {
    size_t padded_args_size = 0;

    /* store the string offset */
    ((size_t*)argv_buf)[i] = offset_to_args_string;

    /* copy the string with '\0' padding */
    const_userptr_t src = (const_userptr_t)((char**)uargs_ptr)[i];
    res = copyinstr(src, args_ptr, argv_string_buf_space_remaining,
                    &padded_args_size);

    if (res) {
      goto SYS_EXEC_ERROR;
    }

    /* fixup the size to be correctly aligned */
    padded_args_size = ALIGNED_SIZE_T(sizeof(char*), padded_args_size);
    argv_string_buf_space_remaining -= padded_args_size;

    /* adjust the args string offset for the next iteration */
    offset_to_args_string += padded_args_size + sizeof(char*);

    /* move location to the next string to be stored */
    args_ptr += padded_args_size;
  }

  /* open the program elf */
  struct vnode* p_vnode;
  res = vfs_open(programname, O_RDONLY, 0, &p_vnode);
  if (res) {
    goto SYS_EXEC_ERROR;
  }

  /* create a new address space */
  struct addrspace* new_as = as_create();
  if (new_as == NULL) {
    vfs_close(p_vnode);
    res = ENOMEM;
    goto SYS_EXEC_ERROR;
  }

  /* switch to the new address space */
  as_deactivate();
  struct addrspace* old_as = proc_setas(new_as);
  as_activate();

  /* Load the executable. */
  vaddr_t entrypoint;
  res = load_elf(p_vnode, &entrypoint);
  if (res) {
    vfs_close(p_vnode);
    goto SYS_EXEC_ERROR;
  }

  vfs_close(p_vnode);

  /* define a new user stack in the address space */
  vaddr_t stackptr;
  res = as_define_stack(new_as, &stackptr);
  if (res) {
    as_deactivate();
    struct addrspace* check_as = proc_setas(old_as);
    KASSERT(check_as == new_as);
    as_destroy(new_as);
    goto SYS_EXEC_ERROR;
  }

  /* copy the argv structure back to userspace and patch the offsets with
   * real addresses */
  stackptr = stackptr - total_argv_size;
  copyout(argv_buf, (userptr_t)(stackptr), total_argv_size);

  for (int i = 0; i < argc; i++) {
    uint32_t* argv_as_uints = (uint32_t*)stackptr;
    argv_as_uints[i] += (uint32_t)(&argv_as_uints[i]);
  }

  kfree(argv_buf);
  argv_buf = NULL;
  userptr_t argv = (userptr_t)stackptr;

  /* dispose of the old address space */
  as_destroy(old_as);

  /* Warp to user mode. */
  enter_new_process(argc, argv, NULL /*userspace addr of environment*/,
                    stackptr, entrypoint);

  goto SYS_EXEC_ERROR_FREE;

SYS_EXEC_ERROR_FREE:
  KASSERT(res == 0);
  return 0;

SYS_EXEC_ERROR:
  KASSERT(res != 0);
  if (argv_buf) {
    kfree(argv_buf);
  }
  return res;
}
