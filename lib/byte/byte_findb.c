#include "../byte.h"
#include "../str.h"

#if LINK_STATIC || !defined(HAVE_MEMMEM)

/* return index to first occurance of data, otherwise return hlen */
size_t
byte_findb(const void* haystack, size_t hlen, const void* what, size_t wlen) {
  size_t last;
  const char* s = (const char*)haystack;

  if(hlen < wlen)
    return hlen;

  last = hlen - wlen;

  for(size_t i = 0; i <= last; i++) {
    if(byte_equal(s, wlen, what))
      return i;

    s++;
  }

  return hlen;
}
#endif