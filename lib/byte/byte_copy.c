#include "byte.h"

/* byte_copy copies in[0] to out[0], in[1] to out[1], ... and in[len-1]
 * to out[len-1]. */
void byte_copy(void* out, size_t len, const void* in) {
  register char* s=out;
  register const char* t=in;
  register const char* u=t+len;
  if (len>127) {
    while ((size_t)s&(sizeof(size_t)-1)) {
      if (t==u) break; *s=*t; ++s; ++t;
    }
    /* s (destination) is now size_t aligned */
#ifndef __i386__
    if (!((size_t)t&(sizeof(size_t)-1)))
#endif
      while (t+sizeof(size_t)<=u) {
	*(size_t*)s=*(size_t*)t;
	s+=sizeof(size_t); t+=sizeof(size_t);
      }
  }
  for (;;) {
    if (t==u) break; *s=*t; ++s; ++t;
    if (t==u) break; *s=*t; ++s; ++t;
    if (t==u) break; *s=*t; ++s; ++t;
    if (t==u) break; *s=*t; ++s; ++t;
  }
}
