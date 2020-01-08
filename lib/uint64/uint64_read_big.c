#define NO_UINT64_MACROS
#include "../uint32.h"
#include "../uint64.h"

uint64
uint64_read_big(const char* in) {
  return ((uint64)uint32_read_big(in) << 32) | uint32_read_big(in + 4);
}
