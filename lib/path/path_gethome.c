#include "../byte.h"
#include "../mmap.h"
#include "../scan.h"
#include "internal.h"
#include "../windoze.h"

#if WINDOWS_NATIVE
#include <shlobj.h>
#endif

/* home directory of uid from /etc/passwd ("name:passwd:uid:gid:gecos:home:shell"),
 * in a static buffer; NULL if there is no such user or no file.
 * Lines without a trailing newline or with "\r\n" are fine; entries
 * whose home does not fit in PATH_MAX are skipped.
 * ----------------------------------------------------------------------- */
char*
path_gethome(int uid) {
  static char home[PATH_MAX + 1];

#if WINDOWS_NATIVE
  PWSTR wpath = NULL;

  /* retrieve user profile directory via the Windows Known Folders API */
  if(FAILED(SHGetKnownFolderPath(&FOLDERID_Profile, 0, NULL, &wpath)))
    return NULL;

  /* convert the utf-16 windows path directly to utf-8 */
  int len = WideCharToMultiByte(CP_UTF8, 0, wpath, -1, home, sizeof(home), NULL, NULL);

  /* free the memory allocated by SHGetKnownFolderPath */
  CoTaskMemFree(wpath);

  if(len <= 0 || (size_t)len > sizeof(home))
    return NULL;

  return home;
#else
  const char *map, *p, *end;
  size_t size, len;
  unsigned int i, id = 0;

  if(!(map = mmap_read("/etc/passwd", &size)))
    return NULL;

  for(i = 0, p = map, end = map + size; p < end; p += len + 1, i++) {
    if(p > map && *(p - 1) == '\n')
      i = 0;

    len = scan_noncharsetnskip(p, &(":\n"[i == 2]), end - p);

    if(i == 5) {
      size_t homelen = len > PATH_MAX ? PATH_MAX : len;

      byte_copy(home, homelen, p);
      home[homelen] = '\0';
      return home;
    }

    if(i == 2) {
      size_t tmplen;

      if((tmplen = scan_uint(p, &id)) > 0 && p[tmplen] == ':')
        if(id == uid)
          len = tmplen;
    }
  }

  mmap_unmap(map, size);
  return NULL;
#endif
}
