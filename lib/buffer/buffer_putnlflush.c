#include "../buffer.h"
#include "../str.h"

ssize_t
buffer_putnlflush(buffer* b) {
  static char nl = '\n';
  return buffer_putflush(b, &nl, 1);
}
