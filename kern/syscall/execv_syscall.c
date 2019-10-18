#include <kern/errno.h>

#include <types.h>

#include <copyinout.h>
#include <lib.h>
#include <limits.h>
#include <syscall.h>

int sys_execv(userptr_t program, userptr_t args)
{
  (void)program;
  (void)args;

  int res = 0;

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
        size_t align_mask = -sizeof(int);
        size_t uargslen = len + 1;
        size_t uargslen_aligned = (uargslen + sizeof(int) - 1) & align_mask;

        bytes_for_aligned_argv_strings += uargslen_aligned;

        break;
      }
      else {
        len++;
      }
    } while (bytes_for_aligned_argv_strings + len <= ARG_MAX);

    argc++;
    uargs_ptr++;
  }

  kprintf("%d arg strings\n", argc);
  kprintf("%d total bytes in aligned string data\n",
          bytes_for_aligned_argv_strings);

  // TODO: Later confirm this logic by reducing argc_limit and ARG_MAX
  if ((argc >= argc_limit) || (bytes_for_aligned_argv_strings > ARG_MAX)) {
    res = E2BIG;
    goto SYS_EXEC_ERROR;
  }

  /*
   * Allocate a structure in kernel space for the argv string data.  Later
   * this buffer will be copied to the correct location in userspace as
   * the argv pointers are being set.
   */

  char* argv_string_buf = (char*)kmalloc(bytes_for_aligned_argv_strings);
  char* argv_string_bufptr = &argv_string_buf[0];
  uargs_ptr = (userptr_t*)args;
  size_t argv_string_buf_space_remaining = bytes_for_aligned_argv_strings;

  memset(argv_string_buf, 0x0, bytes_for_aligned_argv_strings);

  for (int i = 0; i < argc; i++) {
    size_t argspace = 0;
    const_userptr_t p = (const_userptr_t)((char**)uargs_ptr)[i];
    res = copyinstr(p, argv_string_bufptr, argv_string_buf_space_remaining,
                    &argspace);

    if (res) {
      goto SYS_EXEC_ERROR;
    }

    // TODO: make into a helper macro
    size_t align_mask_ = -sizeof(int);
    size_t argvlen_ = argspace;
    size_t argvlen_aligned_ = (argvlen_ + sizeof(int) - 1) & align_mask_;

    argv_string_buf_space_remaining -= argvlen_aligned_;

    // TODO: need ISALIGNED macro
  }

  // TODO: do like in runprogram() and copy stuff into this structure
  // TODO: determine the correct address to use for future helper
  // TODO: copy this onto user's stack and move stack pointer
  // TODO: then do all the other stuff for execv

  // TODO:
  // Copy arguments from user space into kernel buffer
  // Open the executable, create a new address space and load the elf into it
  // Copy the arguments from kernel buffer into user stack
  // Return user mode using enter_new_process

  goto SYS_EXEC_ERROR_FREE;

SYS_EXEC_ERROR_FREE:
  KASSERT(res == 0);
  return 0;

SYS_EXEC_ERROR:
  KASSERT(res != 0);
  return res;
}
