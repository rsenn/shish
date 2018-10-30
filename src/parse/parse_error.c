#include "source.h"
#include "tree.h"
#include "parse.h"
#include "fd.h"
#include "sh.h"

/* parse error message
 * ----------------------------------------------------------------------- */
void *parse_error(struct parser *p, enum tok_flag toks) {
  if(p->tok) {
    sh_msg("unexpected token '");
    
    buffer_putm(fd_err->w, parse_tokname(p->tok, 0), "'", NULL);

    if(toks)
      buffer_putm(fd_err->w, ", expecting '",
                  parse_tokname(toks, 1), "'", NULL);

    buffer_putnlflush(fd_err->w);
  }
  return NULL;
}

