#include "../fd.h"
#include "../fdstack.h"
#include "../../lib/fmt.h"
#include "../../lib/windoze.h"
#if WINDOWS_NATIVE
#include <io.h>
#else
#include <unistd.h>
#endif

/* print an fdtablele entry (mainly for the 'fdtable' builtin)
 * ----------------------------------------------------------------------- */
void
fd_print(struct fd* fd) {
  char numstr[FMT_LONG];
  unsigned int n = 1;

  /* get name if not present */
  if(fd->name == NULL)
    fd_getname(fd);

  /* convert virtual fd to string */
  if(fd->n >= 0)
    n = fmt_long(numstr, fd->n);
  else
    numstr[0] = '-';

  numstr[n] = '\0';

  /* print virtual fd number */
  buffer_putnspace(fd_out->w, 4 - n);
  buffer_puts(fd_out->w, numstr);

  /* convert read fd to string */
  if(fd->r && fd_ok(fd->r->fd))
    n = fmt_long(numstr, fd->r->fd);
  else
    n = 0, numstr[n++] = '-';

  numstr[n] = '\0';

  /* print effective fd number */
  buffer_putnspace(fd_out->w, 5 - n);
  buffer_puts(fd_out->w, numstr);

  /* convert write fd to string */
  if(fd->w && fd_ok(fd->w->fd))
    n = fmt_long(numstr, fd->w->fd);
  else
    n = 0, numstr[n++] = '-';

  numstr[n] = '\0';

  /* print effective fd number */
  buffer_putnspace(fd_out->w, 5 - n);
  buffer_puts(fd_out->w, numstr);

  /* print stack-level */
  n = fmt_long(numstr, fd->stack->level);
  numstr[n] = '\0';
  buffer_putnspace(fd_out->w, 5 - n);
  buffer_puts(fd_out->w, numstr);

  /* print filename */
  buffer_putnspace(fd_out->w, 2);
  buffer_puts(fd_out->w, (fd->name ? fd->name : "-"));

  /* finally put newline */
  buffer_put(fd_out->w, "\n", 1);
}
