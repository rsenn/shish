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

    /* POSIX: a non-interactive shell exits on a syntax error --
       sh_interactive (sh.h), the whole session's own interactive-
       ness, not the source's own type. This used to be gated on
       "is the source a file" (mmap'd, or FD_FILE-backed when
       HAVE_MMAP is off) instead, which wrongly left a "-c" command
       string's syntax error unpunished (falling through to
       sh_loop()'s own post-list check, which can't tell "genuine
       syntax error" apart from "clean EOF right after the last
       command" once both collapse to the same T_EOF token) while
       also wrongly killing an interactive shell outright for a
       syntax error in a `.`-sourced file.
       sh_exit() unwinds subshell frames properly, unlike a raw
       exit(1) -- needed so a syntax error while parsing a sourced
       file from inside a subshell/$(...)/`.` unwinds to the nearest
       enclosing subshell/source instead of killing the whole
       process. */
    if(!sh_interactive)
      sh_exit(1);
  }

  return NULL;
}
