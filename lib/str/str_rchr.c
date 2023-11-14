#include "../str.h"

size_t
str_rchr(const char* s, char needle) {
  const char *in = s, *found = NULL;

  for(;;) {
    if(!*in)
      break;

    if(*in == needle)
      found = in;
    ++in;
  }
  return (size_t)((found ? found : in) - s);
}
