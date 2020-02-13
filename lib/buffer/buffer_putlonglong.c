#include "../buffer.h"
#include "../fmt.h"

int
buffer_putlonglong(buffer* b, int64 i) {
  char buf[FMT_LONG];
  return buffer_put(b, buf, fmt_longlong(buf, i));
}
