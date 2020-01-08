#include "../scan.h"
#include "../str.h"
#include "../uint8.h"

size_t
scan_xmlescape(const char* src, char* dest) {
  size_t n = 0;
  if(*src == '&') {
    ++n;

    if(src[n] == '#') {
      unsigned long code = 0;
      ++n;

      if(src[n] == 'x') {
        ++n;
        n += scan_xlong(&src[n], &code);
      } else {
        n += scan_ulong(&src[n], &code);
      }

      if(src[n] == ';') {
        ++n;
        *dest = code;
        return n;
      }

    } else if(!str_diffn(&src[n], "amp;", 4)) {
      *dest = '&';
      return n + 4;
    } else if(!str_diffn(&src[n], "lt;", 3)) {
      *dest = '<';
      return n + 3;
    } else if(!str_diffn(&src[n], "gt;", 3)) {
      *dest = '>';
      return n + 3;
    } else if(!str_diffn(&src[n], "quot;", 5)) {
      *dest = '"';
      return n + 5;
    } else if(!str_diffn(&src[n], "apos;", 5)) {
      *dest = '\'';
      return n + 5;
    } else if(!str_diffn(&src[n], "nbsp;", 5)) {
      *(uint8*)dest = 160;
      return n + 5;
    } else if(!str_diffn(&src[n], "laquo;", 6)) {
      *(uint8*)dest = 171;
      return n + 6;
    } else if(!str_diffn(&src[n], "raquo;", 6)) {
      *(uint8*)dest = 187;
      return n + 6;
    }
  }

  *dest = *src;
  return 1;
}
