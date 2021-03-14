#include "../../lib/typedefs.h"

size_t
var_len(const char* v) {
  register const char* s;
  for(s = v; *s; ++s)
    if(*s == '=')
      break;
  return s - v;
}
