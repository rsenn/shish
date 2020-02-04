#define _FILE_OFFSET_BITS 64
#include "../open.h"
#include "../windoze.h"
#if WINDOWS_NATIVE
#include <io.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

#ifndef O_RDONLY
#ifdef _A_RDONLY 
#define O_RDONLY _A_RDONLY
#else
#define O_RDONLY 1
#endif
#endif

#ifndef O_NDELAY
#define O_NDELAY 0
#endif

int
open_read(const char* filename) {
  return open(filename, O_RDONLY | O_NDELAY);
}
