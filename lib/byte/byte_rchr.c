#include "../byte.h"

/* byte_rchr returns the largest integer i between 0 and len-1 inclusive
 * such that one[i] equals needle, or len if not found. */
size_t
byte_rchr(const void* haystack, size_t len, char needle) {
  char c = needle;
  const char* s = haystack;
  const char* t = s + len;
  for(;;) {
    --t;
    if(s > t) {
      break;
    };
    if(*t == c)
      return (size_t)(t - s);
  }
  return len;
}
