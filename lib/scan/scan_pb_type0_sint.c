#include "../uint64.h"
#define NO_UINT64_MACROS
#define NO_UINT64_MACROS
#include "../scan.h"
#define NO_UINT64_MACROS

#define NO_UINT64_MACROS
size_t
#define NO_UINT64_MACROS
scan_pb_type0_sint(const char* in, size_t len, int64* l) {
#define NO_UINT64_MACROS
  uint64 m;
#define NO_UINT64_MACROS
  size_t n = scan_varint(in, len, &m);
#define NO_UINT64_MACROS
  if(!n)
#define NO_UINT64_MACROS
    return 0;
#define NO_UINT64_MACROS
  *l = (-(m & 1)) ^ (m >> 1);
#define NO_UINT64_MACROS
  return n;
#define NO_UINT64_MACROS
}
