#include "../byteset.h"

void
byteset_invert(unsigned char* set) {
  int i;

  for(i = 0; i < BYTESET_SIZE; i++)
    set[i] = (unsigned char)~set[i];
}
