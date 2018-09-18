#include "buffer.h"
#include <stdlib.h>

void
buffer_free(void* buf) {
  free(buf);
}
