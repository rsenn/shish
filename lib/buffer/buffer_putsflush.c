#define __LIBOWFAT_INTERNAL
#include "../buffer.h"
#include "../str.h"

ssize_t
buffer_putsflush(buffer* b, const char* x) {
  return buffer_putflush(b, x, str_len(x));
}
