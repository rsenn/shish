#include "../fd.h"
#include "../parse.h"
#include "../sh.h"
#include "../source.h"

/* ----------------------------------------------------------------------- */
void
source_buffer(struct source* s, struct fd* d, const char* x, size_t n) {
  fd_push(d, STDSRC_FILENO, FD_READ);
  fd_string(d, x, n);
  source_push(s);
  s->fd = d;

  /* an in-memory buffer parsed via source_buffer() (eval/expr/trap,
   * backquote or alias re-lexing, prompt escapes) continues whatever
   * source it was spliced out of, not a separate file -- so $LINENO
   * counts from there, not from 1 like source_push()'s default:
   *
   *   sh_errloc    line of the simple command being run ("eval" inside a function body)
   *   parse_lineno line of the top-level list, when no command is running
   *
   * The parent source's live position.line is no use: the parser's lookahead
   * has moved it past the statement by the time a builtin runs. */
  if(s->parent)
    s->position.line = sh_errloc_set ? sh_errloc.line : parse_lineno;
}
