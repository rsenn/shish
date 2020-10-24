#include "../utf8.h"

size_t
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
