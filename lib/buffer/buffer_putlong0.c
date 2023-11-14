#include "../buffer.h"
#include "../fmt.h"

int
buffer_putlong0(buffer* b, long l, int pad) {
  char buf[FMT_LONG];
  ssize_t n = fmt_long(buf, l);

  if(n < pad) {
    if(buffer_putnspace(b, pad - n) < 0)
      return -1;
  }
  return buffer_put(b, buf, n);
}
