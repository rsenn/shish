#include "../fmt.h"
#include "../uint32.h"
#include "../str.h"
#include <sys/types.h>

void fmt_oct3(char* dest, unsigned char w);

size_t
fmt_escapecharshell(char* dest, uint32 ch) {
  char c;
  if(ch > 0xff)
    return 0;
  switch(ch) {
    case 0: str_copy(dest, "\\x00"); return 4;
    case '"': c = '"'; goto doescape;
    case '$': c = '$'; goto doescape;
    case '\a': c = 'a'; goto doescape;
    case '\b': c = 'b'; goto doescape;
    case 0x1b: c = 'e'; goto doescape;
    case '\f': c = 'f'; goto doescape;
    case '\n': c = 'n'; goto doescape;
    case '\r': c = 'r'; goto doescape;
    case '\t': c = 't'; goto doescape;
    case '\v': c = 'v'; goto doescape;
    case '\\':
      c = '\\';
    doescape:
      if(dest) {
        dest[0] = '\\';
        dest[1] = c;
      }
      return 2;
    default:
      if(ch >= 32 /* && ch < 127*/) {
        dest[0] = ch;
        return 1;
      }
      if(dest) {
        dest[0] = '\\';
        dest[1] = 'x';
        dest[2] = fmt_tohex((ch >> 4) & 0xf);
        dest[3] = fmt_tohex(ch & 0xf);
      }
      return 4;
  }
}
