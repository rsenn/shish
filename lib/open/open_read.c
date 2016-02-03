<<<<<<< HEAD
#ifdef WIN32
=======
#ifdef _WIN32
>>>>>>> 6c7455723b47a4989fb5bb621be8f200a306f361
#include <io.h>
#else
#include <unistd.h>
#endif
#include <fcntl.h>
#include "open.h"

#ifndef O_NDELAY
#define O_NDELAY 0
#endif

int open_read(const char *filename) {
  return open(filename,O_RDONLY);
}
