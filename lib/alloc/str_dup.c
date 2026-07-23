#include "../byte.h"
#include "../alloc.h"
#include "../str.h"

void*
str_dup(const char* s) {
  unsigned long n;
  void* ptr;

  n = str_len(s);

  ptr = alloc(n + 1);

  if(ptr)
    byte_copy(ptr, n + 1, s);

  return ptr;
}
