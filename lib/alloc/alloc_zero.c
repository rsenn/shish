#include "../alloc.h"
#include "../byte.h"

void*
alloc_zero(unsigned long size) {
  void* ptr = alloc(size);

  if(ptr)
    byte_zero(ptr, size);
  return ptr;
}
