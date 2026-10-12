#include "../str.h"

/* str_rchrs returns the index of the last occurance of needles[nn] or \0
   in haystack */
size_t
str_rchrs(const char* in, char needles[], size_t nn) {
  const char *t = in, *found = NULL;

  while(*t) {
    for(size_t i = 0; i < nn; ++i)
      if(*t == needles[i])
        found = t;

    ++t;
  }

  return (size_t)((found ? found : t) - in);
}
