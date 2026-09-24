#include "../arena_internal.h"
#include "../byte.h"
#include <stdint.h>

char*
arena_strndup(arena* a, const char* s, size_t len) {
  char* q;

  if(len == SIZE_MAX || !(q = arena_take(a, len + 1, 1)))
    return NULL;

  byte_copy(q, len, s);
  q[len] = '\0';
  return q;
}
