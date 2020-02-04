#include "../windoze.h"
#include "../typedefs.h"

#if WINDOWS_NATIVE
#include "ioctlcmd.h"
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

#if WINDOWS_NATIVE
static int
wcu8len(const wchar_t w) {
  if(!(w & ~0x7f))
    return 1;
  if(!(w & ~0x7ff))
    return 2;
  if(!(w & ~0xffff))
    return 3;
  if(!(w & ~0x1fffff))
    return 4;
  return -1; /* error */
}

static int
wcsu8slen(const wchar_t* pw) {
  int len = 0;
  wchar_t w;

  while((w = *pw++)) {
    if(!(w & ~0x7f))
      len += 1;
    else if(!(w & ~0x7ff))
      len += 2;
    else if(!(w & ~0xffff))
      len += 3;
    else if(!(w & ~0x1fffff))
      len += 4;
    else /* error: add width of null character entity &#x00; */
      len += 6;
  }
  return len;
}

static int
wctou8(char* m, wchar_t w) {
  /* Unicode Table 3-5. UTF-8 Bit Distribution
  Unicode                     1st Byte 2nd Byte 3rd Byte 4th Byte
  00000000 0xxxxxxx           0xxxxxxx
  00000yyy yyxxxxxx           110yyyyy 10xxxxxx
  zzzzyyyy yyxxxxxx           1110zzzz 10yyyyyy 10xxxxxx
  000uuuuu zzzzyyyy yyxxxxxx  11110uuu 10uuzzzz 10yyyyyy 10xxxxxx
  */

  if(!(w & ~0x7f)) {
    m[0] = w & 0x7f;
    m[1] = '\0';
    return 1;
  } else if(!(w & ~0x7ff)) {
    m[0] = ((w >> 6) & 0x1f) | 0xc0;
    m[1] = (w & 0x3f) | 0x80;
    m[2] = '\0';
    return 2;
  } else if(!(w & ~0xffff)) {
    m[0] = ((w >> 12) & 0x0f) | 0xe0;
    m[1] = ((w >> 6) & 0x3f) | 0x80;
    m[2] = (w & 0x3f) | 0x80;
    m[3] = '\0';
    return 3;
  } else if(!(w & ~0x1fffff)) {
    m[0] = ((w >> 18) & 0x07) | 0xf0;
    m[1] = ((w >> 12) & 0x3f) | 0x80;
    m[2] = ((w >> 6) & 0x3f) | 0x80;
    m[3] = (w & 0x3f) | 0x80;
    m[4] = '\0';
    return 4;
  } else
    return -1;
}
static size_t
wcstou8s(char* pu, const wchar_t* pw, size_t count) {
  size_t clen;
  wchar_t w;
  int len = wcsu8slen(pw);

  if(NULL == pu)
    return (size_t)len;

  clen = 0;
  while((w = *pw++)) {
    int ulen = wcu8len(w);

    if(ulen >= 0) {
      if((clen + wcu8len(w)) <= count) {
        clen += wctou8(pu, w);
        pu += ulen;
      } else
        break;
    } else {
      if((clen + 6) <= count) {
        *pu++ = '&';
        *pu++ = '#';
        *pu++ = 'x';
        *pu++ = '0';
        *pu++ = '0';
        *pu++ = ';';
      } else
        break;
    }
  }

  return (size_t)clen;
}

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
