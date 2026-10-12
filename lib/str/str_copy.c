#include "../str.h"

#if LINK_STATIC

/* str_copy copies leading bytes from in to out until \0.
 * returns number of copied bytes. */
size_t
str_copy(char* out, const char* in) {
  char* s = out;
  const char* t = in;

  for(;;) {
    if(!(*s = *t))
      break;

    ++s;
    ++t;
  }

  return (size_t)(s - out);
}
#endif