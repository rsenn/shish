#include "../uint64.h"
#define NO_UINT64_MACROS
#define NO_UINT64_MACROS
#define NO_UINT64_MACROS
#undef NO_UINT64_MACROS
#define NO_UINT64_MACROS
#include "../uint32.h"
#define NO_UINT64_MACROS

#define NO_UINT64_MACROS
void
#define NO_UINT64_MACROS
uint64_pack_big(char* out, uint64 in) {
#define NO_UINT64_MACROS
  uint32_pack_big(out, (uint32)(in >> 32));
#define NO_UINT64_MACROS
  uint32_pack_big(out + 4, in & 0xffffffff);
#define NO_UINT64_MACROS
}
