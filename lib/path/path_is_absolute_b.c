#include "../path_internal.h"
#include "../str.h"
#include "../windoze.h"

/* "/x" is absolute; on Windows so is "\x" and "C:\x" ("C:" and "C:x" stay relative) */
int
path_is_absolute_b(const char* x, size_t n) {
  if(n > 0 && path_issep(x[0]))
    return 1;
  if(n >= 3 && path_isdrive(x) && path_issep(x[2]))
    return 1;
  return 0;
}
