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
     backquote or alias re-lexing, prompt escapes) is a continuation
     of whatever source it was spliced out of, not a separate file --
     so $LINENO tokens found while parsing it should count from
     there, not restart at 1 like source_push()'s default (correct for
     a genuine new file, e.g. the "."/source builtin or the top-level
     script/-c source). Without this, "eval 'echo $LINENO'" always
     printed 1 regardless of where the eval call itself appeared.
     parse_lineno (src/parse/parse_simpletok.c) is the line of
     whichever top-level list sh_loop() is currently executing --
     stamped once per list at parse time, same source $LINENO itself
     ultimately reads back through param->loc.line (see
     expand_param.c). Using the *parent* source's live position.line
     instead would look more direct, but by the time a builtin like
     eval actually runs, that position has already been advanced by
     the parser's own lookahead past the current statement (into
     whatever comes next), which is what caused this to be off by one
     during testing. parse_lineno is not perfectly precise either: for
     eval called from inside a function or loop body, it reflects the
     call site's line, not the line the eval statement itself sits at
     inside that body (see BUGS). */
  if(s->parent)
    s->position.line = parse_lineno;
}
