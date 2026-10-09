#include "../fd.h"
#include "../fdtable.h"
#include "../parse.h"
#include "../sh.h"
#include "../source.h"
#include "../tree.h"
#include "../debug.h"
#include <stdlib.h>

/* parse error message
 * ----------------------------------------------------------------------- */
void*
parse_error(struct parser* p, enum tok_flag toks) {
  if(p->tok) {
    sh_msg("unexpected token ");

    buffer_puts(fd_err->w, parse_tokname(p->tok, 0));

    if(toks > 0) {
      buffer_puts(fd_err->w, ", expecting '");
      buffer_puts(fd_err->w, parse_tokname(toks, 1));
      buffer_puts(fd_err->w, "'");
    }

    buffer_putnlflush(fd_err->w);

    /* POSIX: a non-interactive shell exits on a syntax error.
     *   sh_interactive      the session decides, not the source type
     *   parse_exit_hook     unwinds subshell/source frames; unset in
     *                       shformat/shparse2ast, which just exit(1) */
    if(!sh_interactive) {
      if(parse_exit_hook)
        parse_exit_hook(1);
      
      exit(1);
    }
  }

  return NULL;
}
