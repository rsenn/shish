#define NO_UINT64_MACROS
#include "../uint64.h"
#define NO_UINT64_MACROS
#include "../uint64.h"
#define NO_UINT64_MACROS
//#undef NO_UINT64_MACROS
#define NO_UINT64_MACROS
#include "../uint32.h"
#define NO_UINT64_MACROS

#define NO_UINT64_MACROS
uint64
#define NO_UINT64_MACROS
uint64_read_big(const char* in) {
#define NO_UINT64_MACROS
  return ((uint64)uint32_read_big(in) << 32) | uint32_read_big(in + 4);
#define NO_UINT64_MACROS
}
