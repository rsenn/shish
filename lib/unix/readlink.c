#include "../windoze.h"
#include "../typedefs.h"

#if WINDOWS_NATIVE
#include "../ioctlcmd.h"
#include "../utf8.h"
#include "../stralloc.h"
#include <stdio.h>
#include <windows.h>
#include <winioctl.h>

#ifndef FILE_ATTRIBUTE_REPARSE_POINT
#define FILE_ATTRIBUTE_REPARSE_POINT 0x400
#endif
#ifndef FILE_FLAG_OPEN_REPARSE_POINT
#define FILE_FLAG_OPEN_REPARSE_POINT 0x200000
#endif

#ifndef Newx
#define Newx(v, n, t) v = (t*)malloc((n));
#endif

static BOOL
get_reparse_data(const char* LinkPath, union REPARSE_DATA_BUFFER_UNION* u) {
  HANDLE hFile;
  DWORD returnedLength;

  int attr = GetFileAttributes(LinkPath);

  if(!(attr & FILE_ATTRIBUTE_REPARSE_POINT)) {
    return FALSE;
  }

  hFile = CreateFile(LinkPath, 0, 0, 0, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, 0);

  if(hFile == INVALID_HANDLE_VALUE) {
    return FALSE;
  }

  /* Get the link */
  if(!DeviceIoControl(hFile, FSCTL_GET_REPARSE_POINT, 0, 0, &u->iobuf, 1024, &returnedLength, 0)) {

    CloseHandle(hFile);
    return FALSE;
  }

  CloseHandle(hFile);

  if(u->iobuf.ReparseTag != IO_REPARSE_TAG_MOUNT_POINT && u->iobuf.ReparseTag != IO_REPARSE_TAG_SYMLINK) {
    return FALSE;
  }

  return TRUE;
}

#if WINDOWS_NATIVE
ssize_t
readlink(const char* LinkPath, char* buf, size_t maxlen) {
  union REPARSE_DATA_BUFFER_UNION u;
  wchar_t* wbuf = 0;
  unsigned int u8len, len, wlen;

  if(!get_reparse_data(LinkPath, &u)) {
    return -1;
  }

  switch(u.iobuf.ReparseTag) {
    case IO_REPARSE_TAG_MOUNT_POINT: { /* Junction */
      wbuf = u.iobuf.MountPointReparseBuffer.PathBuffer +
             u.iobuf.MountPointReparseBuffer.SubstituteNameOffset / sizeof(wchar_t);
      wlen = u.iobuf.MountPointReparseBuffer.SubstituteNameLength / sizeof(WCHAR);
      break;
    }
    case IO_REPARSE_TAG_SYMLINK: { /* Symlink */
      wbuf = u.iobuf.SymbolicLinkReparseBuffer.PathBuffer +
             u.iobuf.SymbolicLinkReparseBuffer.SubstituteNameOffset / sizeof(WCHAR);
      wlen = u.iobuf.SymbolicLinkReparseBuffer.SubstituteNameLength / sizeof(WCHAR);
      break;
    }
  }

  if(!wbuf)
    return 0;

  for(len = 0; len < wlen; ++len) {
    u8len += wcu8len(wbuf[len]);

    if(u8len >= maxlen)
      break;
  }
  if(u8len > maxlen) {
    len--;
  }

  u8len = wcstou8s(buf, wbuf, len);
  if(u8len >= maxlen)
    u8len = maxlen - 1;

  buf[u8len] = '\0';
  return u8len;
}
#endif

static DWORD
reparse_tag(const char* LinkPath) {
  union REPARSE_DATA_BUFFER_UNION u;

  if(!get_reparse_data(LinkPath, &u)) {
    return 0;
  }

  return u.iobuf.ReparseTag;
}

int
is_symlink(const char* LinkPath) {
  return reparse_tag(LinkPath) == IO_REPARSE_TAG_SYMLINK;
}

char
is_junction(const char* LinkPath) {
  return reparse_tag(LinkPath) == IO_REPARSE_TAG_MOUNT_POINT;
}

#endif /* WINDOWS */
