#include "../fd.h"
#include "../fdtable.h"
#include "../parse.h"
#include "../sh.h"
#include "../source.h"
#include "../tree.h"
#include "../debug.h"

/* parse error message
 * ----------------------------------------------------------------------- */
void*
parse_error(struct parser* p, enum tok_flag toks) {
  if(p->tok) {
    // source_msg(&source->position);

    sh_msg("unexpected token ");

    /* if(p->tok == T_WORD && p->node && p->node->id == N_ARGSTR) {
       ssize_t len = source->b->p - p->tokstart;

if(len <= 0)
         len = source->b->n - p->tokstart;

if(len > 100)
         len = 10;

       buffer_puts(fd_err->w, "'");
       buffer_put(fd_err->w, &source->b->x[p->tokstart], len);
       buffer_puts(fd_err->w, "' ");
     }
 */
    buffer_puts(fd_err->w, parse_tokname(p->tok, 0));

    if(toks > 0) {
      buffer_puts(fd_err->w, ", expecting '");
      buffer_puts(fd_err->w, parse_tokname(toks, 1));
      buffer_puts(fd_err->w, "'");
    }

    buffer_putnlflush(fd_err->w);

#if 0 // defined(DEBUG_OUTPUT_) && defined(DEBUG_PARSE)
    if(p->node) {
      buffer_puts(fd_err->w, p->tree ? "tree: " : "node: ");
      debug_node(p->tree ? p->tree : p->node, -1);
      buffer_putnlflush(fd_err->w);
    }
#endif

    if(source->b->op == &buffer_dummyreadmmap) {
      exit(1);
    }
  }

  return NULL;
}
