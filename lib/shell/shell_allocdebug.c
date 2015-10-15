#include "shell.h"

extern void *debug_alloc(const char *file, unsigned int line, unsigned long size);

void *shell_allocdebug(const char *file, unsigned int line, unsigned long size) {
  void *ptr = debug_alloc(file, line, size);

  /* exit if failed */
  if(ptr == NULL) {
    shell_error("malloc");
    exit(1);
  }

  /* return pointer otherwise */
  return ptr;
}
