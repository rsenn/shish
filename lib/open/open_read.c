#define _FILE_OFFSET_BITS 64
#include "../windoze.h"
#include "../open.h"
#include <fcntl.h>

#if !WINDOWS_NATIVE
#include <unistd.h>
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

int
open_read(const char* filename) {
  return open(filename, O_RDONLY | O_BINARY);
}
