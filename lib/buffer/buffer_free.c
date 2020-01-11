#include "../buffer.h"
#include <stdlib.h>

void
buffer_free(void* buf) {
  buffer* b = buf;
  free(b->x);
}
