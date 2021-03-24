#ifdef DEBUG_OUTPUT
#include "../fd.h"
#include "../fdstack.h"

void
fdstack_dump(buffer* b) {
  struct fdstack* st;
  struct fd* fd;

  buffer_puts(b,
              "  fd name                                level  e  mode               "
              "              buffer(s)\n");
  buffer_putnc(b, '-', 108);
  buffer_putnlflush(b);

  for(st = fdstack; st; st = st->parent) {
    for(fd = st->list; fd; fd = fd->next) {
      fd_dump(fd, b);
      buffer_putnlflush(b);
    }
  }
  buffer_flush(b);
}
#endif /* DEBUG_OUTPUT */
