#define _FILE_OFFSET_BITS 64
#include "../windoze.h"
#include "../open.h"
#include <fcntl.h>

#if !WINDOWS_NATIVE
#include <unistd.h>
#endif

#ifndef O_NDELAY
#define O_NDELAY 0
#endif

int
open_excl(const char* filename) {
  return open(filename,
              O_WRONLY | O_TRUNC | O_CREAT | O_EXCL
#if !WINDOWS_NATIVE
              ,
              0600
#endif
  );
}
