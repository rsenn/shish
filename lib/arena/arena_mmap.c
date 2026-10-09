#include "../arena.h"
#include "../mmap.h"

static void*
mmap_get(size_t* size) {
  *size = (*size + 4095) & ~(size_t)4095;
  return mmap_anon(*size);
}

static void
mmap_put(void* p, size_t size) {
  mmap_unmap(p, size);
}

const struct arena_src arena_mmap = {mmap_get, mmap_put};
