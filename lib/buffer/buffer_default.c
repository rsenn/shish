#include "../buffer.h"

/* set sane defaults + operation */
void
buffer_default(buffer* b, buffer_op_fn* op) {
  b->fd = -1;
  b->p = 0;
  b->n = 0;
  b->op = (buffer_op_proto*)op;
}
