#include "../buffer.h"
#include "../fmt.h"
#include <stdlib.h>

int
buffer_putxlong0(buffer* b, unsigned long l, int pad) {
  char buf[FMT_XLONG];
  size_t n = fmt_xlong(buf, l);

  if(buffer_putnc(b, '0', pad - n) < 0)
    return -1;

  return buffer_put(b, buf, n);
}
