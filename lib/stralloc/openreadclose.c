#include "../stralloc.h"
#include "../open.h"
#include <errno.h>

int
openreadclose(const char* fn, stralloc* sa, size_t bufsize) {
  int fd = open_read(fn);

  if(fd == -1) {
    if(errno == ENOENT)
      return 0;

    return -1;
  }

  if(readclose(fd, sa, bufsize) == -1)
    return -1;

  return 1;
}
