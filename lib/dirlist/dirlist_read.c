#include "../windoze.h"
#include "dirlist_internal.h"
#include <errno.h>

#if WINDOWS_NATIVE
#include "../utf8.h"
#include <windows.h>

#ifndef FIND_FIRST_EX_LARGE_FETCH
#define FIND_FIRST_EX_LARGE_FETCH 2
#endif
#ifndef FindExInfoBasic
#define FindExInfoBasic ((FINDEX_INFO_LEVELS)1)
#endif

static int
dirlist_type_of(DWORD attr) {
  if(attr & FILE_ATTRIBUTE_REPARSE_POINT)
    return DL_LNK;
  return attr & FILE_ATTRIBUTE_DIRECTORY ? DL_DIR : DL_REG;
}

static int
dirlist_errno(DWORD e) {
  switch(e) {
    case ERROR_ACCESS_DENIED: return EACCES;
    case ERROR_DIRECTORY: return ENOTDIR;
    case ERROR_FILE_NOT_FOUND:
    case ERROR_PATH_NOT_FOUND:
    case ERROR_INVALID_NAME: return ENOENT;
    case ERROR_NOT_ENOUGH_MEMORY:
    case ERROR_OUTOFMEMORY: return ENOMEM;
  }
  return EIO;
}

/* UTF-8 path, "\*" appended; the wide copy is scratch in the arena */
static HANDLE
dirlist_open(arena* a, const char* path, WIN32_FIND_DATAW* fd) {
  size_t n = str_len(path), w;
  unsigned short* wp;
  HANDLE h;

  if(!(wp = arena_allocn(a, sizeof(unsigned short), n + 4, __alignof__(unsigned short)))) {
    errno = ENOMEM;
    return INVALID_HANDLE_VALUE;
  }

  w = u8tou16(wp, n, path, n, NULL);
  wp[w] = 0;
  n = w;
  if(w && wp[w - 1] != '/' && wp[w - 1] != '\\' && wp[w - 1] != ':')
    wp[w++] = '\\';
  wp[w++] = '*';
  wp[w] = 0;

  h = FindFirstFileExW((const WCHAR*)wp, FindExInfoBasic, fd, FindExSearchNameMatch, NULL, FIND_FIRST_EX_LARGE_FETCH);
  if(h == INVALID_HANDLE_VALUE) {
    DWORD e = GetLastError(), attr;

    /* "file\*" fails with a code that varies (PATH_NOT_FOUND, INVALID_NAME,
       DIRECTORY): a path that exists but is no directory is ENOTDIR */
    wp[n] = 0;
    if(e != ERROR_ACCESS_DENIED && (attr = GetFileAttributesW((const WCHAR*)wp)) != INVALID_FILE_ATTRIBUTES &&
       !(attr & FILE_ATTRIBUTE_DIRECTORY))
      e = ERROR_DIRECTORY;
    errno = dirlist_errno(e);
  }
  return h;
}
#else
#include <dirent.h>
#endif

struct dirlist
dirlist_read(arena* a, const char* path, unsigned flags) {
  static const struct dirlist fail;
  struct dirlist_buf b = {{0, 0, 0}, 256};
  arena_pos pos = arena_tell(a);
  int err = 0;

#if WINDOWS_NATIVE
  WIN32_FIND_DATAW fd;
  HANDLE h = dirlist_open(a, path, &fd);

  arena_rewind(a, pos);
  if(h == INVALID_HANDLE_VALUE)
    return fail;
  pos = arena_tell(a);
#else
  DIR* dir = opendir(path);
  struct dirent* de;

  if(!dir)
    return fail;
#endif

  if(!(b.d.s = arena_alloc(a, b.cap, 1)))
    err = ENOMEM;

#if WINDOWS_NATIVE
  while(!err) {
    char name[4 * MAX_PATH];
    size_t n = u8fromu16(name, sizeof(name) - 1, (const unsigned short*)fd.cFileName, lstrlenW(fd.cFileName), NULL);

    name[n] = '\0';
    if(!dirlist_skip(name, flags))
      if(dirlist_add(a, &b, dirlist_type_of(fd.dwFileAttributes), name, n))
        err = ENOMEM;
    if(!FindNextFileW(h, &fd)) {
      if(GetLastError() != ERROR_NO_MORE_FILES)
        err = dirlist_errno(GetLastError());
      break;
    }
  }
  FindClose(h);
#else
  while(!err) {
    int t = DL_UNKNOWN;

    errno = 0;
    if(!(de = readdir(dir))) {
      err = errno;
      break;
    }
    if(dirlist_skip(de->d_name, flags))
      continue;
#ifdef DT_UNKNOWN
    switch(de->d_type) {
      case DT_REG: t = DL_REG; break;
      case DT_DIR: t = DL_DIR; break;
      case DT_LNK: t = DL_LNK; break;
      case DT_UNKNOWN: break;
      default: t = DL_OTHER; break;
    }
#endif
    if(dirlist_add(a, &b, t, de->d_name, str_len(de->d_name)))
      err = ENOMEM;
  }
  closedir(dir);
#endif

  if(err) {
    arena_rewind(a, pos);
    errno = err;
    return fail;
  }

  /* freeze: give the slack back; an empty blob still owns one byte */
  arena_trim(a, b.d.s, b.cap, b.d.len ? b.d.len : 1);

  if((flags & DIRLIST_SORT) && dirlist_sort(a, &b.d, NULL)) {
    arena_rewind(a, pos);
    return fail;
  }

  return b.d;
}
