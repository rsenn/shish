#include "../path_internal.h"
#include "../str.h"
#include "../windoze.h"
#include <ctype.h>

int
path_is_absolute_b(const char* x, size_t n) {
  if(n > 0 && x[0] == '/')
    return 1;
#if WINDOWS
  if(n >= 3 && isalnum(x[0]) && x[1] == ':' && path_issep(x[2]))
    return 1;
#endif
  return 0;
}
