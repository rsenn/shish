#include <sys/types.h>
<<<<<<< HEAD
#ifndef WIN32
#include <unistd.h>
#endif
#ifdef WIN32
=======
#ifdef _WIN32
>>>>>>> 6c7455723b47a4989fb5bb621be8f200a306f361
#include <windows.h>
#else
#include <sys/mman.h>
#endif
#include "open.h"
#include "mmap.h"

int mmap_unmap(char* mapped,unsigned long maplen) {
<<<<<<< HEAD
#ifdef WIN32
=======
#ifdef _WIN32
>>>>>>> 6c7455723b47a4989fb5bb621be8f200a306f361
  (void)maplen;
  return UnmapViewOfFile(mapped)?0:-1;
#else
  return munmap(mapped,maplen);
#endif
}
