#include "../uint64.h"

#undef uint64_unpack
void
uint64_unpack(const char* in, uint64* out) {
  *out = uint64_read(in);
}
