#define _FILE_OFFSET_BITS 64
#include "open.h"
#include <fcntl.h>
#include <unistd.h>

#ifndef O_NDELAY
#define O_NDELAY 0
#endif

int
open_read(const char* filename) {
  return open(filename, O_RDONLY | O_NDELAY);
}
