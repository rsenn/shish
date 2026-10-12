#include "../byte.h"

#if LINK_STATIC

/* byte_copy copies in[0] to out[0], in[1] to out[1], ... and in[len - 1]
 * to out[len - 1]. */
void
byte_copy(void* out, size_t len, const void* in) {
  char* s = out;
  const char* t = in;

  for(size_t i = 0; i < len; ++i)
    s[i] = t[i];
}
#endif
