#include "../buffer.h"
#include "../fmt.h"
#include "../uint64.h"

int
buffer_putulonglong(buffer* b, uint64 i) {
  char buf[FMT_ULONG];
  return buffer_put(b, buf, fmt_ulonglong(buf, i));
}
