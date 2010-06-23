#include "source.h"
#include "fd.h"

/* ----------------------------------------------------------------------- */
void source_flush(void)
{
  register buffer *b = source->b;
  b->p = 0;
  b->n = 0;
}

