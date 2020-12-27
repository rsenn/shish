#include "../buffer.h"
#include "../str.h"

int
buffer_putspad(buffer* b, const char* x, size_t pad) {
  size_t len = str_len(x);
  if(len > 0) {
    if(buffer_put(b, x, len) < 0)
      return -1;
  }
  if(len < pad) {
    if(buffer_putnspace(b, pad - len) < 0)
      return -1;
  }
  return 0;
}
