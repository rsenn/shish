#include "../byte.h"

/* returns the index of the first byte that is not needle */
size_t
byte_chrn(const void* haystack, size_t len, char needle) {
  const char *s = haystack, *t = haystack + len;

  for(;;) {
    if(s == t)
      break;

    if(*s != needle)
      break;

    ++s;
  }

  return s - (const char*)haystack;
}