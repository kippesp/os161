#ifndef ALIGN_H
#define ALIGN_H

#include <types.h>

/* helper function to align on type_size */
static inline size_t ALIGNED_SIZE_T(size_t type_size, size_t bytes)
{
  size_t align_mask = -type_size;
  size_t aligned_bytes = (bytes + type_size - 1) & align_mask;

  return aligned_bytes;
}

/* helper function to check alignment of userptr_t */
static inline bool IS_ALIGNED_USERPTR_T(const_userptr_t ptr_in)
{
  size_t ptr = (size_t)ptr_in;

  /* the types should permit the math to be okay here */
  KASSERT(sizeof(size_t) >= sizeof(const_userptr_t));

  size_t ptr_size = sizeof(const_userptr_t);
  size_t align_mask = -sizeof(ptr_size);
  size_t aligned_ptr = (ptr + ptr_size - 1) & align_mask;

  return (const_userptr_t)aligned_ptr == ptr_in;
}

#endif
