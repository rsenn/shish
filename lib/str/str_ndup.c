#include "../str.h"
#include "../byte.h"
#include "../alloc.h"

char*
str_ndup(const char* in, size_t len) {
  char* t;

  if(!(t = alloc(len + 1)))
    return t;

  byte_copy(t, len, in);
  t[len] = '\0';
  return t;
}
