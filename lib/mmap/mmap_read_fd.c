#include "../windoze.h"
#include "../mmap.h"
#include "../open.h"

#if WINDOWS_NATIVE
#include <windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#endif

char mmap_empty[] = {0};

/* an empty file maps to mmap_empty (success, *filesize == 0) */
char*
mmap_read_fd(fd_t fd, size_t* filesize) {
#if WINDOWS_NATIVE
  HANDLE h = (HANDLE)_get_osfhandle((int)fd);
  LARGE_INTEGER sz;
  HANDLE m;
  char* map = 0;

  if(h == INVALID_HANDLE_VALUE || !GetFileSizeEx(h, &sz))
    return 0;

  if(!(*filesize = (size_t)sz.QuadPart))
    return mmap_empty;

  if((m = CreateFileMapping(h, 0, PAGE_READONLY, 0, 0, NULL))) {
    map = MapViewOfFile(m, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(m);
  }

  return map;
#else
  struct stat st;
  char* map;

  if(fstat(fd, &st) != 0)
    return 0;

  if(!(*filesize = st.st_size))
    return mmap_empty;

  map = mmap(0, *filesize, PROT_READ, MAP_SHARED, fd, 0);
  return map == (char*)-1 ? 0 : map;
#endif
}
