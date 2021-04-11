#include "../buffer.h"
#include <stdlib.h>

void
buffer_free(buffer* b) {
  free(b->x);
}
