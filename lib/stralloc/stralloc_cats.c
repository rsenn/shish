#include "../str.h"
#include "../stralloc.h"

extern int
stralloc_cats(stralloc* sa, const char* buf) {
  return stralloc_catb(sa, buf, str_len(buf));
}
