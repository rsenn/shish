#include "../byte.h"

#if LINK_STATIC || !defined(HAVE_MEMCCPY)

/* byte_ccopy copies in[0] to out[0], n[1] to out[1], ....
   until character c is encountered, returns number of bytes
   copied, including the character c. */
size_t
byte_ccopy(void* out, size_t len, const void* in, char c) {
  char* t = out;
  const char* s = in;

  while(len--) {
    *t++ = *s;

    if(*s == c)
      return s + 1 - (const char*)in;

    ++s;
  }

  return s - (const char*)in;
}
#endif