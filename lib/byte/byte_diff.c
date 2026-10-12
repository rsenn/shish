#include "../byte.h"

#if LINK_STATIC

/* byte_diff returns negative, 0, or positive, depending on whether the
 * string one[0], one[1], ..., one[len - 1] is lexicographically smaller
 * than, equal to, or greater than the string one[0], one[1], ...,
 * one[len - 1]. When the strings are different, byte_diff does not read
 * bytes past the first difference. */
int
byte_diff(const void* a, size_t len, const void* b) {
  int r;

  for(size_t i = 0; i < len; ++i)
    if((r = ((unsigned char*)a)[i] - ((unsigned char*)b)[i]))
      return r;

  return 0;
}
#endif
