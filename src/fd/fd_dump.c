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
  char numbuf[FMT_LONG];
  int out = 0;

  if(fd->name == NULL)
    fd_getname(fd);

  /* file descriptor */
  n = fmt_long(numbuf, fd->n);
  buffer_putnspace(b, 4 - n);
  buffer_put(b, numbuf, n);
  buffer_putspace(b);

  /* name */
  n = fd->name ? str_len(fd->name) : 0;
  if(n > 14) {
    buffer_put(b, fd->name, 12);
    buffer_puts(b, "...");
  } else {
    buffer_put(b, fd->name, n);
    buffer_putnspace(b, 15 - n);
  }

  /* level */
  n = fmt_long(numbuf, fd->stack->level);
  buffer_putnspace(b, 3 - n);
  buffer_put(b, numbuf, n);
  buffer_putspace(b);

  /* buffers */
  buffer_dump(b, fd->r);
  buffer_putspace(b);
  buffer_dump(b, fd->w);
  buffer_putspace(b);

  /* flags */
  for(n = 0; n < sizeof(fd_flags) / sizeof(fd_flags[0]); n++) {
    if(fd->mode & (1 << n)) {
      if(out++)
        buffer_putc(b, '|');
      buffer_puts(b, fd_flags[n]);
    }
  }

  buffer_putnlflush(b);
}
#endif /* DEBUG_OUTPUT */
