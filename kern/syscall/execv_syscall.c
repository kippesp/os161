#include <types.h>

#include <addrspace.h>
#include <copyinout.h>
#include <current.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <lib.h>
#include <limits.h>
#include <proc.h>
#include <syscall.h>
#include <vfs.h>

#include <align.h>

int sys_execv(userptr_t programs_in, userptr_t uargs_in)
{
  int res = 0;
  char* argv_buf = NULL;      /* char** args in kernel space */
  char* new_p_name = NULL;    /* name for new process */
  size_t total_argv_size = 0; /* everything; strings, ptrs, and NULL */
  size_t bytes_for_aligned_argv_strings = 0; /* just the padded strings */
  int argc = 0;

  if (programs_in == NULL) {
    res = EFAULT;
    goto SYS_EXEC_ERROR;
  }

  char programs[512];
  memset(programs, 0x0, sizeof(programs));
  res = copyinstr(programs_in, programs, sizeof(programs) - 1, NULL);
  if (res) {
    goto SYS_EXEC_ERROR;
  }

  if (programs[0] == '\0') {
    res = EINVAL;
    goto SYS_EXEC_ERROR;
  }

  /*
   * Conceptually execv operates on the char** args from the user in two parts.
   * The first half of execv identifies the arg strings to be copied from
   * userspace into kernel space.  The does sets up a new address space and
   * formats the arg strings onto a new stack of the new address space.
   */

  {
    /*
     * Find the lengths of the arguments and check any other issues.  Since we
     * don't know the size of the buffer we need to create, there is little
     * else we can do other than scan all the strings to check the total
     * length.  If at any point we exceed ARG_MAX we can error out.
     */
    const int MY_ARGC_LIMIT = 4096; /* 4K of arguments seems a generous limit */

    userptr_t* uargs_ptr = (userptr_t*)uargs_in;
    userptr_t uargs;
    res = copyin((const_userptr_t)uargs_ptr, &uargs, sizeof(userptr_t*));
    if (res) {
      goto SYS_EXEC_ERROR;
    }

    /* char** args (i.e. uargs_in copied to uargs) from the user is okay,
     * but each char* args (i.e. uargs[n]) is unsafe.  Make uargs[n] safe
     * by first copying the pointer into uargs. */

    while ((uargs != NULL) && (argc < MY_ARGC_LIMIT)) {
      int len = 0;

      /* before copying the string to kernel space, scan its contents to
       * determine its int-aligned length */
      char* args_substr = (char*)uargs;
      do {
        /* clearly there is a better way to get the string length before
         * doing the actual copy; but the pointer can become invalid
         * anywhere up until a '\0' terminator is found */
        char termcheck;
        res = copyin((const_userptr_t)uargs + len, &termcheck, sizeof(char));
        if (res) {
          goto SYS_EXEC_ERROR;
        }

        /* reached '\0' at end of string? */
        if (termcheck == '\0') {
          bytes_for_aligned_argv_strings +=
              ALIGNED_SIZE_T(sizeof(int), len + 1);

          break;
        }
        else {
          args_substr++;
          len++;
        }
      } while (bytes_for_aligned_argv_strings + len <= ARG_MAX);

      uargs_ptr++;
      argc++;

      res = copyin((const_userptr_t)uargs_ptr, &uargs, sizeof(userptr_t*));
      if (res) {
        goto SYS_EXEC_ERROR;
      }
    }

    if ((argc > MY_ARGC_LIMIT) || (bytes_for_aligned_argv_strings > ARG_MAX)) {
      res = E2BIG;
      goto SYS_EXEC_ERROR;
    }

    /* check size won't go over the limit defined for this implementation */
    total_argv_size =
        bytes_for_aligned_argv_strings + (argc + 1) * sizeof(char*);
    if (total_argv_size > ARG_MAX) {
      res = E2BIG;
      goto SYS_EXEC_ERROR;
    }
  }

  vaddr_t stackptr = 0x0;   /* stack that will be defined for the new process */
  vaddr_t entrypoint = 0x0; /* the elf */

  {
    /*
     * Allocate a structure in kernel space in order to build the argv
     * structure.  Since the stack pointer isn't defined until later in the
     * function, some patchup will be done after the data has been copied to
     * userspace.  To be valid, each offsets (see below) will be updated with
     * absolute addresses to its argv string.  See runprogram() for the layout
     * definition of arv once it is copied to userspace.  The layout while in
     * kernelspace is here:
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

    /* Create the name of the new process */
    new_p_name = kstrdup(programs);

    KASSERT(sizeof(int) == sizeof(userptr_t));

    argv_buf = (char*)kmalloc(total_argv_size);
    memset(argv_buf, 0x0, total_argv_size);

    /* pointer to the string; initially argv[5] */
    char* args_ptr = argv_buf + (argc + 1) * sizeof(char*);

    /* upper bound of string space */
    size_t argv_string_buf_space_remaining = bytes_for_aligned_argv_strings;

    /* offset to uargs_in; later will be patched with the absolute pointer */
    ssize_t offset_to_args_string = 2 * sizeof(char*);

    /* set uargs_ptr to the first userspace uargs_in pointer */
    userptr_t* uargs_ptr = (userptr_t*)uargs_in;

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

      /* adjust the uargs_in string offset for the next iteration */
      offset_to_args_string += padded_args_size + sizeof(char*);

      /* move location to the next string to be stored */
      args_ptr += padded_args_size;
    }

    /* open the program elf */
    struct vnode* p_vnode;
    res = vfs_open(programs, O_RDONLY, 0, &p_vnode);
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
    res = load_elf(p_vnode, &entrypoint);
    if (res) {
      vfs_close(p_vnode);
      goto SYS_EXEC_ERROR;
    }

    vfs_close(p_vnode);

    /* define a new user stack in the address space */
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

    /* dispose of the old address space */
    as_destroy(old_as);
  }

  kfree(argv_buf);
  argv_buf = NULL;
  userptr_t argv = (userptr_t)stackptr;

  /* give the new process its new name */
  struct proc* proc = curproc;
  kfree(proc->p_name);
  proc->p_name = new_p_name;

  /* Warp to user mode. */
  enter_new_process(argc, argv, NULL /*userspace addr of environment*/,
                    stackptr, entrypoint);

  /* enter_new_process does not return. */
  panic("enter_new_process returned\n");

SYS_EXEC_ERROR:
  KASSERT(res != 0);
  if (new_p_name)
    kfree(new_p_name);
  if (argv_buf) {
    kfree(argv_buf);
  }
  return res;
}
