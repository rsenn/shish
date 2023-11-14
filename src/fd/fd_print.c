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
fd_print(struct fd* d, buffer* b) {
  char numstr[FMT_LONG];
  unsigned int n = 1;

  /* get name if not present */
  if(d->name == NULL)
    fd_getname(d);

  /* convert virtual d to string */
  if(d->n >= 0)
    n = fmt_long(numstr, d->n);
  else
    numstr[0] = '-';

  numstr[n] = '\0';

  /* print virtual d number */
  buffer_putnspace(b, 4 - n);
  buffer_puts(b, numstr);

  /* convert read d to string */
  if(d->r && fd_ok(d->r->fd))
    n = fmt_long(numstr, d->r->fd);
  else
    n = 0, numstr[n++] = '-';

  numstr[n] = '\0';

  /* print effective d number */
  buffer_putnspace(b, 5 - n);
  buffer_puts(b, numstr);

  /* convert write d to string */
  if(d->w && fd_ok(d->w->fd))
    n = fmt_long(numstr, d->w->fd);
  else
    n = 0, numstr[n++] = '-';

  numstr[n] = '\0';

  /* print effective d number */
  buffer_putnspace(b, 5 - n);
  buffer_puts(b, numstr);

  /* print stack-level */
  n = fmt_long(numstr, d->stack->level);
  numstr[n] = '\0';
  buffer_putnspace(b, 5 - n);
  buffer_puts(b, numstr);

  /* print filename */
  buffer_putnspace(b, 2);
  buffer_puts(b, (d->name ? d->name : "-"));

  /* finally put newline */
  buffer_put(b, "\n", 1);
}
