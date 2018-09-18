#include "shell.h"

#ifndef DEBUG

void*
shell_alloc(unsigned long size) {
  void* ptr = malloc(size);

  /* exit if failed */
  if(ptr == NULL) {
    shell_error("malloc");
    exit(1);
  }

  /* return pointer otherwise */
  return ptr;
}
#endif /* !defined(DEBUG) */