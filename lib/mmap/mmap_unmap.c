#include "../windoze.h"
#include "../mmap.h"

#if WINDOWS_NATIVE
#include <windows.h>

int
mmap_unmap(const char* mapped, size_t maplen) {
  (void)maplen;
  return UnmapViewOfFile(mapped) ? 0 : -1;
}
#else
#include <sys/mman.h>

int
mmap_unmap(const char* mapped, size_t maplen) {
  if(!maplen)
    return 0;

  return munmap((void*)mapped, maplen);
}
#endif
