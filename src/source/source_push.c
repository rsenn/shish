#include "../fd.h"
#include "../source.h"
#include "../term.h"

struct source* source = 0;

/* ----------------------------------------------------------------------- */
void
source_push(struct source* s) {
  s->parent = source;
  s->position.line = 1;
  s->position.column = 1;
  s->mode = 0;
  s->b = fd_src->r;

  source = s;

#if !defined(SHFORMAT) && !defined(SHPARSE2AST)
  if(fd_src->mode & FD_CHAR) {
    if(term_init(fd_src, fd_err))
      source->mode |= SOURCE_IACTIVE;
  }
#endif
}
