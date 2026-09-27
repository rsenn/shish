#include "../byteset.h"

int
byteset_range(unsigned char* set, unsigned lo, unsigned hi) {
  unsigned c;

  if(lo > hi)
    return -1;

  for(c = lo; c <= hi && c < 256; c++)
    byteset_add(set, c);

  return 0;
}
