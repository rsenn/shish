#include "../str.h"

size_t
str_chr(const char* in, char needle) {
  const char *t = in, c = needle;

  for(;;) {
    if(!*t || *t == c)
      break;

    ++t;
  }

  return (size_t)(t - in);
}
