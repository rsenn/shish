#include "../byteset.h"

void
byteset_add(unsigned char* set, unsigned c) {
  c &= 0xff;
  set[c >> 3] |= (unsigned char)(1u << (c & 7));
}
