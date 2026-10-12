#include "../str.h"

/* str_chrn returns the index of the first character that is not c */
size_t
str_chrn(const char* in, char c) {
  const char* t = in;

  while(*t) {
    if(*t != c)
      break;

    ++t;
  }

  return (size_t)(t - in);
}
