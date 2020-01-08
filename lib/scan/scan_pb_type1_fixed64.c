#define NO_UINT64_MACROS
#include "../uint64.h"
#define NO_UINT64_MACROS
#include "../scan.h"
#define NO_UINT64_MACROS

#define NO_UINT64_MACROS
size_t
#define NO_UINT64_MACROS
scan_pb_type1_fixed64(const char* in, size_t len, uint64* d) {
#define NO_UINT64_MACROS
  if(len < 8)
#define NO_UINT64_MACROS
    return 0;
#define NO_UINT64_MACROS
  uint64_unpack(in, d);
#define NO_UINT64_MACROS
  return 8;
#define NO_UINT64_MACROS
}
