#include "../buffer.h"
#include "../fmt.h"

int
buffer_putxlonglong(buffer* b, uint64 l) {
  char buf[FMT_XLONG];
  return buffer_put(b, buf, fmt_xlonglong(buf, l));
}
