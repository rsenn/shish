#include "../fd.h"
#include "../source.h"

/* ----------------------------------------------------------------------- */
void
source_flush(void) {
  register buffer* b = source->b;
  b->p = 0;
  b->n = 0;
}
