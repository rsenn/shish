#include "shell.h"
#include "byte.h"
#include "str.h"

void *shell_strdup(const char *s) {
  unsigned long n;
  void *ptr;

  n = str_len(s);

  ptr = shell_alloc(n + 1);

  if(ptr)
    byte_copy(ptr, n + 1, s);

  return ptr;
}
