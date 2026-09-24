#define _DEFAULT_SOURCE 1
#define _GNU_SOURCE 1

#include "../arena.h"

#ifndef _WIN32
#include <sys/mman.h>

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

static void*
mmap_get(size_t* size) {
  void* p;

  *size = (*size + 4095) & ~(size_t)4095;
  p = mmap(NULL, *size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  return p == MAP_FAILED ? NULL : p;
}

static void
mmap_put(void* p, size_t size) {
  munmap(p, size);
}

const struct arena_src arena_mmap = {mmap_get, mmap_put};
#endif
