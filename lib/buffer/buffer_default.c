#include "buffer.h"

/* set sane defaults + operation */
void buffer_default(buffer *b, int (*op)())
{
  b->fd = -1;
  b->p = 0;
  b->n = 0;
  b->op = op;
}
