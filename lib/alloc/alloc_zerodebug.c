#include "../alloc.h"
#include "../byte.h"

#ifdef DEBUG_ALLOC
extern void* debug_alloc(const char* file,
                         unsigned int line,
                         unsigned long size);

void*
alloc_zerodebug(const char* file, unsigned int line, unsigned long size) {
  void* ptr = debug_alloc(file, line, size);
  if(ptr)
    byte_zero(ptr, size);
  return ptr;
}
#endif /* defined(DEBUG_ALLOC) */
