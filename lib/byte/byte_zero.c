#include "../byte.h"

#if LINK_STATIC

/* byte_zero sets the bytes out[0], out[1], ..., out[len - 1] to 0 */
void
byte_zero(void* out, size_t len) {
  char* s = out;
  const char* t = out + len;

  while(s != t)
    *s++ = '\0';
}
#endif
