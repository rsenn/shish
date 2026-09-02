#include "../sh.h"
#include "../../lib/scan.h"

/* decode backslash escapes (as understood by `echo -e` and ANSI-C
 * ($'...') quoting) from src[0..len) into dst, returning the decoded
 * length. dst must be at least len bytes; decoding never grows the
 * string. src[len] must be readable (a NUL terminator satisfies
 * this) since a trailing '\' peeks one byte past len to decide it
 * has no escape to complete.
 *
 *   \n \r \t \v \b \f \e \\   single-char escapes
 *   \0NNN                     octal (scan_8int)
 *   \xHH                      hex (scan_xchar)
 *   anything else             '\' kept literal, next byte untouched
 * ----------------------------------------------------------------------- */
size_t
sh_unescape(const char* src, size_t len, char* dst) {
  size_t k = 0, j;

  for(j = 0; j < len; j++) {
    int ch = (unsigned char)src[j];

    if(ch == '\\') {
      size_t n = 1;

      switch(src[j + 1]) {
        case 'x':
          if((n = scan_xchar(&src[j + 2], (unsigned char*)&ch)) > 0)
            ++n;
          break;

        case '0': n = scan_8int(&src[j + 1], (unsigned int*)&ch); break;

        case 'E':
        case 'e': ch = '\033'; break;
        case 'f': ch = '\014'; break;
        case 'n': ch = '\n'; break;
        case 'r': ch = '\r'; break;
        case 't': ch = '\t'; break;
        case 'v': ch = '\v'; break;
        case 'b': ch = '\010'; break;
        case '\\': ch = '\\'; break;
        default: n = 0; break;
      }

      if(n > 0) {
        dst[k++] = ch;
        j += n;
        continue;
      }
    }

    dst[k++] = ch;
  }

  return k;
}
