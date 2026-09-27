#include "../byteset.h"

int
byteset_next(const unsigned char* set, int from) {
  int c;

  for(c = from < 0 ? 0 : from; c < 256; c++)
    if(byteset_has(set, (unsigned)c))
      return c;

  return -1;
}
