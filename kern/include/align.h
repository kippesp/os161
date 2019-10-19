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

#endif
