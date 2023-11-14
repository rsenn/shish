#include "../buffer.h"
#include "../byte.h"
#include "../scan.h"

int
buffer_get_until(buffer* b, char* x, size_t len, const char* charset, size_t setlen) {
  size_t blen;

  for(blen = 0; blen < len;) {
    int r;

    if((r = buffer_getc(b, x)) < 0)
      return r;

    if(r == 0) {
      *x = 0;
      break;
    }
    blen++;

    if(byte_chr(charset, setlen, *x++) < setlen) {
      break;
    };
  }
  return blen;
}
