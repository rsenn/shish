#define __LIBOWFAT_INTERNAL
#define NO_BUILTINS 1
#include "../buffer.h"
#include "../str.h"

int
buffer_puts(buffer* b, const char* x) {
  return buffer_put(b, x, str_len(x));
}
