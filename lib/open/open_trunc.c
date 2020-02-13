#define _FILE_OFFSET_BITS 64
#include "../windoze.h"

#if WINDOWS_NATIVE
#ifdef _MSC_VER
#define _CRT_INTERNAL_NONSTDC_NAMES 1
#endif
#include <io.h>
#if !defined(__LCC__) && !defined(__MINGW32__)
#define read _read
#define write _write
#define open _open
#define close _close
#endif
#else
#include <unistd.h>
#endif

#include "../open.h"
#include <fcntl.h>

#ifndef O_NDELAY
#define O_NDELAY 0
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

int
open_trunc(const char* filename) {
  return open(filename,
              O_WRONLY | O_TRUNC | O_CREAT | O_BINARY
#if !defined(__BORLANDC__)
              ,
              0644
#endif
  );
}
