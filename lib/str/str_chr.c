#include "../str.h"

/* str_chr returns the index of the first occurance of needle or \0
   in haystack */
size_t
str_chr(const char* in, char needle) {
  const char *t = in, c = needle;

  while(*t) {
    if(*t == c)
      break;

    ++t;
  }

  return (size_t)(t - in);
}
