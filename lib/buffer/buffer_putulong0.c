#include "../buffer.h"
#include "../fmt.h"

int
buffer_putulong0(buffer* b, unsigned long l, int pad) {
  char buf[FMT_ULONG];
  ssize_t n = fmt_ulong(buf, l);
  if(n < pad) {
    if(buffer_putnspace(b, pad - n) < 0)
      return -1;
  }
  return buffer_put(b, buf, n);
}
