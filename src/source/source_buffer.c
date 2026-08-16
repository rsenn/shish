#include "../fd.h"
#include "../parse.h"
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
   * should count from there, not restart at 1 like source_push()'s
   * default.
   *
   * parse_lineno is the line of whichever top-level list sh_loop() is
   * currently executing, stamped once per list at parse time. The
   * parent source's live position.line would look more direct, but by
   * the time a builtin like eval runs, that position has already been
   * advanced past the current statement by the parser's own
   * lookahead. */
  if(s->parent)
    s->position.line = parse_lineno;
}
