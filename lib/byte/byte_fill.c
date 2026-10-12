#include "../byte.h"

#if LINK_STATIC

/* byte_fill sets the bytes out[0], out[1], ..., out[len - 1] to c */
void
byte_fill(void* out, size_t len, int c) {
  char* s = out;
  const char* t = s + len;

  while(s != t)
    *s++ = c;
}
#endif
