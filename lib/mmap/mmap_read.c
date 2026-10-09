#include "../windoze.h"
#include "../mmap.h"
#include "../open.h"

#if !WINDOWS_NATIVE
#include <unistd.h>
#endif

const char*
mmap_read(const char* filename, size_t* filesize) {
  int fd = open_read(filename);
  const char* map;

  if(fd < 0)
    return 0;

  map = mmap_read_fd(fd, filesize);
  close(fd);
  return map;
}
