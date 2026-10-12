#include "../str.h"

/* str_rchr returns the index of the last occurance of needle or \0
   in haystack  */
size_t
str_rchr(const char* in, char needle) {
  const char *t = in, *found = NULL;

  while(*t) {
    if(*t == needle)
      found = t;

    ++t;
  }

  return (size_t)((found ? found : t) - in);
}
