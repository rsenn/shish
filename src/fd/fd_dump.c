#ifdef DEBUG_OUTPUT
#include "../../lib/buffer.h"
#include "../fd.h"
#include "../fdstack.h"
#include "../../lib/fmt.h"
#include "../../lib/str.h"

static const char* fd_flags[] = {"READ",
                                 "WRITE",
                                 "APPEND",
                                 "EXCL",
                                 "TRUNC",
                                 NULL,
                                 NULL,
                                 NULL,
                                 "FILE",
                                 "DIR",
                                 "LINK",
                                 "CHAR",
                                 "BLOCK",
                                 "SOCKET",
                                 "PIPE",
                                 "STRALLOC",
                                 "STRING",
                                 "DUP",
                                 "TERM"};

/* dump out debugging info about an (fd)
 * ----------------------------------------------------------------------- */
void
fd_dump(struct fd* fd, buffer* b) {
  unsigned long n;
  int out = 0;

  if(fd->name == NULL)
    fd_getname(fd);

  /* file descriptor */
  buffer_puts(b, "fd#");
  buffer_putlong(b, fd->n);
  buffer_putspace(b);

  /* name */
  n = fd->name ? str_len(fd->name) : 0;
  if(n > 14) {
    buffer_put(b, fd->name, 12);
    buffer_puts(b, "...");
  } else {
    buffer_put(b, fd->name, n);
  }

  /* level */
    buffer_putulong0(b, fd->stack->level, 5);
  buffer_puts(b, " e=");
  buffer_putlong(b, fd->e);
  buffer_puts(b, " mode=");

  /* flags */
  for(n = 0; n < sizeof(fd_flags) / sizeof(fd_flags[0]); n++) {
    if(fd->mode & (1 << n)) {
      if(out++)
        buffer_putc(b, '|');
      //buffer_puts(b, "FD_");
      buffer_puts(b, fd_flags[n]);
    }
  }
  buffer_putspace(b);

  /* buffers */
  if(fd->r && (fd->mode & FD_READ)) {
    buffer_puts(b, "r=");
    buffer_dump(b, fd->r);
    buffer_putspace(b);
  }
  if(fd->w && (fd->mode & FD_WRITE)) {
    buffer_puts(b, "w=");

    buffer_dump(b, fd->w);
    buffer_putspace(b);
  }

  buffer_putnlflush(b);
}
#endif /* DEBUG_OUTPUT */
