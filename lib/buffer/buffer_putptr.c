#include "../buffer.h"
#include "../fmt.h"

size_t buffer_putptr_size_2 = sizeof(void*) * 2;

int
buffer_putptr(buffer* b, void* ptr) {
  char buf[FMT_XLONG + 1];
  size_t n;

  if(ptr == NULL)
    return buffer_puts(b, "(null)");

  n = fmt_xlonglong(buf, (uint64)(intptr_t)ptr);
  buf[n] = '\0';
  // buffer_put(b, "0x", 2);

  while(n++ < buffer_putptr_size_2)
    buffer_putc(b, '0');
  buffer_puts(b, buf);
  return n + 2;
}
