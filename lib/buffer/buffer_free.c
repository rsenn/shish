#include "../alloc.h"
#include "../buffer.h"
#include <stdlib.h>

void
buffer_free(buffer* b) {
  alloc_free(b->x);
}
