#include "../parse.h"
#include "../source.h"
#include "../sh.h"
#include "../../lib/fmt.h"

void
parse_dump(struct parser* p, buffer* b) {

  buffer_puts(b, sh_argv0);
  buffer_puts(b, ":");
  buffer_putulong(b, p->tokstart.line);
  buffer_puts(b, ":");
  buffer_putulong(b, p->tokstart.column);
  buffer_puts(b, ": Got token ");
  if(p->flags) {
    char buf[8];
    buffer_puts(b, "(");
    if(p->flags & P_BQUOTE)
      buffer_puts(b, "P_BQUOTE ");
    if(p->flags & P_NOKEYWD)
      buffer_puts(b, "P_NOKEYWD ");
    if(p->flags & P_NOASSIGN)
      buffer_puts(b, "P_NOASSIGN ");
    if(p->flags & ~(P_BQUOTE | P_NOKEYWD | P_NOASSIGN))
      buffer_put(b, buf, fmt_xlong(buf, p->flags & ~(P_BQUOTE | P_NOKEYWD | P_NOASSIGN)));
    if(b->p > 0 && b->x[b->p - 1] == ' ')
      b->p--;
    buffer_puts(b, ") ");
  }
  buffer_puts(b, parse_tokname(p->tok, 0));
  buffer_putnlflush(b);
}