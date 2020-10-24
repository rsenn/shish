#include "../utf8.h"

int
u8swcslen(const char* pu) {
  int len = 0;
  char c;

  while((c = *pu)) {
    if(!(c & 0x80)) {
      len++;
      pu += 1;
    } else if((c & 0xe0) == 0xc0) {
      len++;
      pu += 2;
    } else if((c & 0xf0) == 0xe0) {
      len++;
      pu += 3;
    } else if((c & 0xf8) == 0xf0) {
      len++;
      pu += 4;
    } else /* error: add width of single byte character entity &#xFF; */ {
      len += 6;
      pu += 1;
    }
  }
  return len;
}
