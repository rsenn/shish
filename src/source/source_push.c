#include "../fdtable.h"
#include "../source.h"
#include "../term.h"
#include "../sh.h"

/* ----------------------------------------------------------------------- */
void
source_push(struct source* s) {
  s->parent = source;
  s->position.line = 1;
  s->position.column = 1;
  s->position.offset = 0;
  s->mode = 0;
  s->b = fd_src->r;

  source = s;
}
