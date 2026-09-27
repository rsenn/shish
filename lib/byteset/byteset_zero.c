#include "../byteset.h"
#include "../byte.h"

void
byteset_zero(unsigned char* set) {
  byte_zero(set, BYTESET_SIZE);
}
