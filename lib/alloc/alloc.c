#include "../alloc.h"
#include "../shell.h"

#ifndef DEBUG_ALLOC

void*
alloc(unsigned long size) {
  void* ptr = malloc(size);

  /* exit if failed */
  if(ptr == NULL) {
    shell_error("malloc");
    exit(1);
  }

  /* return pointer otherwise */
  return ptr;
}
#endif /* !defined(DEBUG_ALLOC) */
