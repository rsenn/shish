#include "../alloc.h"
#include "../shell.h"

#ifndef DEBUG_ALLOC

void*
alloc_re(void* ptr, unsigned long size) {
  void* newptr;

  if(ptr == NULL)
    return alloc(size);

  newptr = realloc(ptr, size);

  /* exit if failed */
  if(newptr == NULL) {
    shell_error("realloc");
    exit(1);
  }

  /* return pointer otherwise */
  return newptr;
}
#endif /* !defined(DEBUG_ALLOC) */
