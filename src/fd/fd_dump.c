#include "../debug.h"

#if defined(DEBUG_OUTPUT) /*&& (defined(DEBUG_FDTABLE) || \
                             defined(DEBUG_FDSTACK) || defined(DEBUG_FD))*/
#include "../../lib/buffer.h"
#include "../term.h"
#include "../fd.h"
#include "../fdstack.h"
#include "../../lib/fmt.h"
#include "../../lib/str.h"

/* dump out debugging info about an (fd)
 * ----------------------------------------------------------------------- */
void
fd_dump(struct fd* d, buffer* b) {
  if(d->name == NULL)
    fd_getname(d);

  /* file descriptor */
  buffer_puts(b, COLOR_MAGENTA);
  buffer_putlong0(b, d->n, 4);
  buffer_puts(b, COLOR_NONE " ");

  /* name */
  buffer_putspad(b, d->name ? d->name : "NULL", 10);

  /* level */
  term_escape(b, 42, 'G');

  buffer_putulong0(b, d->stack->level, 2);
  buffer_putnspace(b, 2);

  buffer_putlong0(b, d->e, 3);
  buffer_putnspace(b, 2);

  dump_flags(b,
             d->mode,
             (const char* const[]){
                 "READ",   "WRITE", "APPEND",   "EXCL",    "TRUNC", 0,        0,        0,
                 "FILE",   "DIR",   "LINK",     "CHAR",    "BLOCK", "SOCKET", "PIPE",   "STRALLOC",
                 "STRING", "DUP",   "TERM",     "NULL",    0,       0,        0,        0,
                 "FLUSH",  "CLOSE", "FREENAME", "DUPNAME", "FREE",  0,        "TMPBUF", "OPEN",
             },
             32);

  term_escape(b, 84, 'G');

  /* buffers */
  if(d->r && (d->mode & FD_READ)) {
    buffer_puts(b, "r=");
    buffer_dump(b, d->r);
    buffer_putspace(b);
  }

  if(d->w && (d->mode & FD_WRITE)) {
    buffer_puts(b, "w=");

    buffer_dump(b, d->w);
    buffer_putspace(b);
  }
}
#endif /* DEBUG_OUTPUT */
