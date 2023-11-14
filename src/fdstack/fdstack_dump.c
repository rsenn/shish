#if defined(DEBUG_OUTPUT) //&& defined(DEBUG_FDSTACK) && defined(DEBUG_FD)
#include "../fd.h"
#include "../fdstack.h"

void
fdstack_dump(buffer* b) {
  struct fdstack* st;
  struct fd* fd;

  buffer_puts(b, "  fd name\t\t\t\tlevel  e  mode\t\t\t\t   buffer(s)\n");
  buffer_putnc(b, '-', 108);
  buffer_putnlflush(b);

  for(st = fdstack; st; st = st->parent) {
    for(fd = st->list; fd; fd = fd->next) {
      fd_dump(fd, b);
      buffer_putnlflush(b);
    }
  }

  buffer_putnlflush(b);
}
#endif /* DEBUG_OUTPUT */
