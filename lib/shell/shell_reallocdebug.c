#include "../shell.h"

#ifdef DEBUG_ALLOC
extern void* debug_realloc(const char* file, unsigned int line, void* ptr, unsigned long size);

void*
shell_reallocdebug(const char* file, unsigned int line, void* ptr, unsigned long size) {
  void* newptr;

  if(ptr == NULL)
    return shell_allocdebug(file, line, size);

  newptr = debug_realloc(file, line, ptr, size);

  /* exit if failed */
  if(newptr == NULL) {
    shell_error("realloc");
    exit(1);
  }

  /* return pointer otherwise */
  return newptr;
}
#endif /* defined(DEBUG_ALLOC) */