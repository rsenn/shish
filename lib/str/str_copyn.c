#include "../str.h"

#if LINK_STATIC || !defined(HAVE_MEMCCPY)

/* str_copyb copies leading bytes from in to out until \0
   or max bytes reached. returns number of copied bytes. */
size_t
str_copyn(void* out, const void* in, size_t max) {
  char* s = out;
  const char* t = in;
  size_t len = 0;

  while(max-- > 0) {
    if(!(*s = *t))
      return len;

    ++s;
    ++t;
    ++len;
  }

  *s = '\0';
  return len;
}
#endif