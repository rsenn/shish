#include <sys/types.h>
#include <unistd.h>
#ifdef __MINGW32__
#include <windows.h>
#else
#include <sys/mman.h>
#endif
#include "open.h"
#include "mmap.h"

extern char* mmap_read(const char* filename,unsigned long* filesize) {
  char *map;
#ifdef __MINGW32__
#define close CloseHandle
  HANDLE fd;
  fd=CreateFile(filename,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
  if (fd==INVALID_HANDLE_VALUE)
    return 0;
#else
  int fd=open_read(filename);
  if(fd == -1)
    return 0;
#endif
  map = mmap_read_fd(fd, filesize);
  close(fd);
  return map;
}
