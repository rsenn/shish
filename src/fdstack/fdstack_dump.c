#ifdef DEBUG_OUTPUT
#include "../fd.h"
#include "../fdstack.h"

void
fdstack_dump(buffer* b) {
  struct fdstack* st;
  struct fd* fd;

  buffer_puts(b,
              "  fd  name        level read-buffer                          "
              "write-buffer                        flags\n");
  buffer_puts(b,
              "----------------------------------------------------------------"
              "--------------------------------------------\n");
  for(st = fdstack; st; st = st->parent) {
    for(fd = st->list; fd; fd = fd->next) {
      fd_dump(fd, b);
    }
  }
  buffer_flush(b);
}
#endif /* DEBUG_OUTPUT */
