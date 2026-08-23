#include "../byte.h"

/* byte_copyr copies in[0..len) to out[0..len), like byte_copy(), but
 * is safe when the two ranges overlap (memmove() semantics): out >
 * in copies back-to-front, out < in copies front-to-back, so the
 * write side never clobbers bytes the read side hasn't reached yet.
 * ----------------------------------------------------------------------- */
#if LINK_STATIC
void
byte_copyr(void* out, size_t len, const void* in) {
  char* o = (char*)out;
  const char* i = (const char*)in;

  if(o < i) {
    size_t k;

    for(k = 0; k < len; k++)
      o[k] = i[k];
  } else if(o > i) {
    while(len) {
      len--;
      o[len] = i[len];
    }
  }
}
#endif
