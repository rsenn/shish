#include "fd.h"
#include "parse.h"
#include "sh.h"
#include "source.h"
#include "tree.h"

/* parse error message
 * ----------------------------------------------------------------------- */
void*
parse_error(struct parser* p, enum tok_flag toks) {
  if(p->tok) {
    sh_msg("unexpected token '");

    buffer_puts(fd_err->w, parse_tokname(p->tok, 0));
    buffer_putc(fd_err->w, '\'');

    if(toks) {
      buffer_puts(fd_err->w, ", expecting '");
      buffer_puts(fd_err->w, parse_tokname(toks, 1));
      buffer_putc(fd_err->w, '\'');
    }

    buffer_putnlflush(fd_err->w);
  }
  return NULL;
}
