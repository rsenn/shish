#ifdef DEBUG_OUTPUT
#include "../../lib/buffer.h"
#include "../fd.h"
#include "../fdstack.h"
#include "../debug.h"
#include "../../lib/fmt.h"
#include "../../lib/str.h"

/* dump out debugging info about an (fd)
 * ----------------------------------------------------------------------- */
void
fd_dump(struct fd* fd, buffer* b) {
  ssize_t pos, len;

  if(fd->name == NULL)
    fd_getname(fd);

  /* file descriptor */
  buffer_putlong0(b, fd->n, 4);
  buffer_putspace(b);

  /* name */
  buffer_putspad(b, fd->name, 36);

  /* level */
  buffer_putulong0(b, fd->stack->level, 2);
  buffer_putnspace(b, 3);

  buffer_putlong0(b, fd->e, 3);
  buffer_putnspace(b, 2);

  dump_flags(b,
             fd->mode,
             (const char* const[]){"READ",   "WRITE",  "APPEND",   "EXCL",
                                   "TRUNC",  0,        0,          0,
                                   "FILE",   "DIR",    "LINK",     "CHAR",
                                   "BLOCK",  "SOCKET", "PIPE",     "STRALLOC",
                                   "STRING", "DUP",    "TERM",     "NULL",
                                   0,        0,        0,          0,
                                   "FLUSH",  "CLOSE",  "FREENAME", "DUPNAME",
                                   "FREE",   0,        "TMPBUF",   "OPEN"}, 32);



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
}
#endif /* DEBUG_OUTPUT */
