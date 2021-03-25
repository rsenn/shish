#include "../uint64.h"
#include "../buffer.h"
#include "../stralloc.h"
#include "../fmt.h"

void
stralloc_dump(const stralloc* sa, buffer* b) {
  size_t len = 0;
  buffer_puts(b, "{ s=");
  buffer_putptr(b, sa->s);
  if(sa->s) {
    char buf[16] = {'\\'};
    size_t i;
    buffer_puts(b, " \"");
    len = sa->len < 100 ? sa->len : 100;
    for(i = 0; i < len; i++) {
      uint32 c = (unsigned int)(unsigned char)sa->s[i];
      if(c < 0x20 || c == '"') {
        if(c == '\r' || c == '\n' || c == '\t' || c == '\v')
          buffer_put(b, buf, fmt_escapecharc(buf, c));
        else
          buffer_put(b, buf, 1 + fmt_8long(buf + 1, c));
      } else {
        buffer_putc(b, c);
      }
    }
    buffer_puts(b, "\"");
  }
  if(len < sa->len)
    buffer_puts(b, "... ");
  buffer_puts(b, ", len=");
  buffer_putulonglong(b, sa->len);
  buffer_puts(b, ", a=");
  buffer_putulonglong(b, sa->a);
  buffer_puts(b, " }");
}
