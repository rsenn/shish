#include "../fd.h"
#include "../parse.h"
#include "../sh.h"
#include "../source.h"
#include "../tree.h"

/* parse error message
 * ----------------------------------------------------------------------- */
void*
parse_error(struct parser* p, enum tok_flag toks) {
  if(p->tok) {
    source_msg();
    sh_msg("unexpected token ");

    if(p->tok == T_WORD && p->node && p->node->id == N_ARGSTR) {
      buffer_puts(fd_err->w, "'");
      buffer_putsa(fd_err->w, p->tree ? &p->tree->nargstr.stra : &p->node->narg.stra);
      buffer_puts(fd_err->w, "' ");
    }

    buffer_puts(fd_err->w, parse_tokname(p->tok, 0));

    if(toks > 0) {
      buffer_puts(fd_err->w, ", expecting '");
      buffer_puts(fd_err->w, parse_tokname(toks, 1));
      buffer_puts(fd_err->w, "'");
    }

    buffer_putnlflush(fd_err->w);
  }
  return NULL;
}
