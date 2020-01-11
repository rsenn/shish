#include "../byte.h"
#include "../shell.h"
#include "../str.h"

#ifdef DEBUG_ALLOC

extern void* shell_allocdebug(const char* file, unsigned int line, unsigned long n);

void*
shell_strdupdebug(const char* file, unsigned int line, const char* s) {
  unsigned long n;
  void* ptr;

  n = str_len(s);

  ptr = shell_allocdebug(file, line, n + 1);

  if(ptr)
    byte_copy(ptr, n + 1, s);

  return ptr;
}
#endif /* defined(DEBUG_ALLOC) */
