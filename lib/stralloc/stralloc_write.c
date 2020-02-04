#include "../stralloc.h"
#include "../buffer.h"

ssize_t
stralloc_write(stralloc* sa, const char* buf, size_t len, void* ptr) {
  buffer* b = ptr;
  return stralloc_catb(b->cookie, buf, len) ? len : -1;
}
