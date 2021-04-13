#include "../alloc.h"
#include "../byte.h"

#ifndef DEBUG_ALLOC

void*
alloc_zero(unsigned long size) {
  void* ptr = alloc(size);
  if(ptr)
    byte_zero(ptr, size);
  return ptr;
}
#endif /* !defined(DEBUG_ALLOC) */
