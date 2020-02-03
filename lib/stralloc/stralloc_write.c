#include "../typedefs.h"
#include "../stralloc.h"

ssize_t
stralloc_write(stralloc* sa, const char* buf, size_t len) {
  return stralloc_catb(sa, buf, len) ? len : -1;
}
