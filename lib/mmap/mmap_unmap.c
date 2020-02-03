#include <sys/types.h>
#ifdef _WIN32
#include <windows.h>
#else
#include "../open.h"
#include <sys/mman.h>
#include <unistd.h>
#endif
#include "../mmap.h"

int
mmap_unmap(const char* mapped, size_t maplen) {
#ifdef _WIN32
  (void)maplen;
  return UnmapViewOfFile(mapped) ? 0 : -1;
#else
  return maplen ? munmap((char*)mapped, maplen) : 0;
#endif
}
