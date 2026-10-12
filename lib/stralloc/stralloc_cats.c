#include "../str.h"
#include "../stralloc.h"

int
stralloc_cats(stralloc* sa, const char* buf) {
  return stralloc_catb(sa, buf, str_len(buf));
}
