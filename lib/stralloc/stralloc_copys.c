#include "../str.h"
#include "../stralloc.h"

extern int
stralloc_copys(stralloc* sa, const char* buf) {
  return stralloc_copyb(sa, buf, str_len(buf));
}
