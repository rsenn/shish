#include <sys/types.h>
#ifndef WIN32
#include <unistd.h>
#endif
#ifdef WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif
#include "open.h"
#include "mmap.h"

int mmap_unmap(char* mapped,unsigned long maplen) {
#ifdef WIN32
  (void)maplen;
  return UnmapViewOfFile(mapped)?0:-1;
#else
  return munmap(mapped,maplen);
#endif
}
