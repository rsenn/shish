#include "../uint64.h"
#define NO_UINT64_MACROS
#define NO_UINT64_MACROS
#include "../scan.h"
#define NO_UINT64_MACROS

#define NO_UINT64_MACROS
size_t
#define NO_UINT64_MACROS
scan_pb_tag(const char* in, size_t len, size_t* fieldno, unsigned char* type) {
#define NO_UINT64_MACROS
  uint64 l;
#define NO_UINT64_MACROS
  size_t n = scan_varint(in, len, &l);
#define NO_UINT64_MACROS
  if(n == 0)
#define NO_UINT64_MACROS
    return 0;
#define NO_UINT64_MACROS
  *type = l & 7;
#define NO_UINT64_MACROS
  *fieldno = (l >> 3);
#define NO_UINT64_MACROS
  return n;
#define NO_UINT64_MACROS
}
