#include "../stralloc.h"
#include "../open.h"

#if WINDOWS_NATIVE
#ifdef _MSC_VER
#define _CRT_INTERNAL_NONSTDC_NAMES 1
#endif
#include <io.h>
#if !defined(__LCC__) && !defined(__MINGW32__)
#define close _close
#endif
#else
#include <unistd.h>
#endif

ssize_t
readclose(int fd, stralloc* sa, size_t bufsize) {
  if(!stralloc_copys(sa, "")) {
    close(fd);
    return -1;
  }

  return readclose_append(fd, sa, bufsize);
}
