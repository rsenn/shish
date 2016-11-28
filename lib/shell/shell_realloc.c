#include "shell.h"

#ifndef DEBUG

void *shell_realloc(void *ptr, unsigned long size) {
  void *newptr;

  if(ptr == NULL)
    return shell_alloc(size);

  newptr = realloc(ptr, size);

  /* exit if failed */
  if(newptr == NULL) {
    shell_error("realloc");
    exit(1);
  }

  /* return pointer otherwise */
  return newptr;
}
#endif /* !defined(DEBUG) */