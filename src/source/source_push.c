#include "../fd.h"
#include "../source.h"
#include "../term.h"

struct source* source = 0;

/* ----------------------------------------------------------------------- */
void
source_push(struct source* src) {
  src->parent = source;
  src->position.line = 1;
  src->position.column = 1;
  src->mode = 0;
  src->b = fd_src->r;

  source = src;

#if !defined(SHFORMAT) && !defined(SHPARSE2AST)
  if(fd_src->mode & FD_CHAR) {
    if(term_init(fd_src, fd_err))
      source->mode |= SOURCE_IACTIVE;
  }
#endif
}
