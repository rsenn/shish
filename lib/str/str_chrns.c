#include "../str.h"

/* str_nchrs returns the index of the first occurance of a character
   not in needles[nn] or \0 in haystack */
size_t
str_chrns(const char* in, const char needles[], size_t nn) {
  const char* t = in;

  while(*t) {
    size_t i;

    for(i = 0; i < nn; i++)
      if(*t == needles[i])
        break;

    if(i == nn)
      break;

    ++t;
  }

  return (size_t)(t - in);
}
