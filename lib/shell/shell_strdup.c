#include "byte.h"
#include "shell.h"
#include "str.h"

#ifndef DEBUG

void*
shell_strdup(const char* s) {
  unsigned long n;
  void* ptr;

  n = str_len(s);

  ptr = shell_alloc(n + 1);

  if(ptr) byte_copy(ptr, n + 1, s);

  return ptr;
}
#endif /* !defined(DEBUG) */