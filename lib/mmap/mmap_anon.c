#include "../windoze.h"
#include "../mmap.h"

#if WINDOWS_NATIVE
#include <windows.h>
#else
#include <sys/mman.h>

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif
#endif

/* private, zero-filled, read/write mapping of len bytes; release with mmap_unmap() */
char*
mmap_anon(size_t len) {
#if WINDOWS_NATIVE
  /* pagefile-backed section, so UnmapViewOfFile() (mmap_unmap) releases it */
  DWORD hi = (DWORD)((unsigned long long)len >> 32);
  HANDLE m = CreateFileMapping(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, hi, (DWORD)len, NULL);
  char* map;

  if(!m)
    return 0;

  map = MapViewOfFile(m, FILE_MAP_WRITE, 0, 0, 0);
  CloseHandle(m);
  return map;
#else
  char* map = mmap(0, len, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

  return map == (char*)-1 ? 0 : map;
#endif
}
