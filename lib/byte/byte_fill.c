#include "byte.h"

/* byte_fill sets the bytes out[0], out[1], ..., out[len-1] to c */
void byte_fill(void* out, unsigned long len, int c) {
  register char* s=out;
  register const char* t=s+len;
  for (;;) {
    if (s==t) break; *s=c; ++s;
    if (s==t) break; *s=c; ++s;
    if (s==t) break; *s=c; ++s;
    if (s==t) break; *s=c; ++s;
  }
}
