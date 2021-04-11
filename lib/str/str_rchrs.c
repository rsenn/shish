#include "../str.h"

size_t
str_rchrs(const char* in, char needles[], size_t nn) {
  const char *s = in, *found = NULL;
  size_t i;
  for(;;) {
    if(!*s)
      break;
    for(i = 0; i < nn; ++i) {
      if(*s == needles[i])
        found = s;
    }
    ++s;
  }
  return (size_t)((found ? found : s) - in);
}
