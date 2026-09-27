#include "../byteset.h"

int
byteset_has(const unsigned char* set, unsigned c) {
  c &= 0xff;
  return (set[c >> 3] >> (c & 7)) & 1;
}
