#include "../byte.h"

#if LINK_STATIC || !defined(HAVE_MEMRCHR)

/* returns the index of the first byte from the end that is needle */
size_t
byte_rchr(const void* haystack, size_t len, char needle) {
  const char *s = (const char*)haystack, *t = s + len;

  for(;;) {
    --t;

    if(s > t)
      break;

    if(*t == needle)
      return (size_t)(t - s);
  }

  return len;
}
#endif