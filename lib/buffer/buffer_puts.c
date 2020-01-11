#define __LIBOWFAT_INTERNAL
#include "../buffer.h"
#include "../str.h"

int
buffer_puts(buffer* b, const char* x) {
  return buffer_put(b, x, str_len(x));
}
