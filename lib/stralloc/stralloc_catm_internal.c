#include "../str.h"
#include "../stralloc.h"
#include <stdarg.h>
#include <string.h>
#undef stralloc_catm_internal

int
stralloc_catm_internal(stralloc* sa, ...) {
  va_list a;
  const char* s;
  size_t n = 0;
  va_start(a, sa);
  while((s = va_arg(a, const char*)))
    n += str_len(s);
  va_end(a);
  stralloc_readyplus(sa, n);

  va_start(a, sa);
  while((s = va_arg(a, const char*)))
    if(stralloc_cats(sa, s) == 0) {
      va_end(a);
      return 0;
    }
  va_end(a);
  return 1;
}
