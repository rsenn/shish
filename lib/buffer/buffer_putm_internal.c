#include "../buffer.h"
#include <stdarg.h>

int
buffer_putm_internal(buffer* b, ...) {
  int r = 0;
  va_list a;
  const char* s;
  va_start(a, b);
  while((s = va_arg(a, const char*)))
    if(buffer_puts(b, s) == -1) {
      r = -1;
      break;
    }
  va_end(a);
  return r;
}
