#include "../str.h"

#if LINK_STATIC

/* str_len returns the length of the string in bytes */
size_t
str_len(const char* in) {
  const char* t = in;

  while(*t)
    ++t;

  return t - in;
}
#endif
