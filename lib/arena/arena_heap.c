#include "../arena.h"
#include <stdlib.h>

static void*
heap_get(size_t* size) {
  return malloc(*size);
}

static void
heap_put(void* p, size_t size) {
  (void)size;
  free(p);
}

const struct arena_src arena_heap = {heap_get, heap_put};
