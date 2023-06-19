#include "../alloc.h"
#include "../shell.h"

#ifdef DEBUG_ALLOC

extern void* debug_alloc(const char* file,
                         unsigned int line,
                         unsigned long size);

void*
allocdebug(const char* file, unsigned int line, unsigned long size) {
  void* ptr = debug_alloc(file, line, size);

  /* exit if failed */
  if(ptr == NULL) {
    shell_error("malloc");
    exit(1);
  }

  /* return pointer otherwise */
  return ptr;
}
#endif /* defined(DEBUG_ALLOC) */
