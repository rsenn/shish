#include "../utf8.h"

size_t
u8stowcs(wchar_t* pw, const char* pu, size_t count) {
  size_t clen = 0;

  if(NULL == pw)
    return u8swcslen(pu);

  while(*pu && clen < count) {
    int ulen = u8towc(&pw[clen], pu, 1);
    if(ulen < 0)
      return (size_t)-1;
    else {
      clen++;
      pu += ulen;
    }
  }
  if('\0' == *pu && clen < count)
    pw[clen++] = L'\0';
  return clen;
}
