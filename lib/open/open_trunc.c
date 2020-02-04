#define _FILE_OFFSET_BITS 64
#include "../open.h"
#include "../windoze.h"
#include <fcntl.h>
#if WINDOWS_NATIVE
#include <io.h>
#else
#include <unistd.h>
#endif

#ifndef O_NDELAY
#define O_NDELAY 0
#endif

int
open_trunc(const char* filename) {
  return open(filename, O_WRONLY | O_NDELAY | O_TRUNC | O_CREAT, 0644);
}
