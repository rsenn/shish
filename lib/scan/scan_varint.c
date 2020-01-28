#include "../uint64.h"
#define NO_UINT64_MACROS
#define NO_UINT64_MACROS
#define NO_UINT64_MACROS
#include "../scan.h"
#define NO_UINT64_MACROS

#define NO_UINT64_MACROS
size_t
#define NO_UINT64_MACROS
scan_varint(const char* in, size_t len, uint64* n) {
#define NO_UINT64_MACROS
  size_t i;
#define NO_UINT64_MACROS
  uint64 l;
#define NO_UINT64_MACROS
  if(len == 0)
#define NO_UINT64_MACROS
    return 0;
#define NO_UINT64_MACROS
  l = 0;
#define NO_UINT64_MACROS
  for(l = 0, i = 0; i < len; ++i) {
#define NO_UINT64_MACROS
    l += (in[i] & 0x7f) << (i * 7);
#define NO_UINT64_MACROS
    if(!(in[i] & 0x80)) {
#define NO_UINT64_MACROS
      *n = l;
#define NO_UINT64_MACROS
      return i + 1;
#define NO_UINT64_MACROS
    }
#define NO_UINT64_MACROS
  }
#define NO_UINT64_MACROS
  return 0;
#define NO_UINT64_MACROS
}
#define NO_UINT64_MACROS

#define NO_UINT64_MACROS
size_t scan_pb_type0_int(const char* dest, size_t len, uint64* l)
#define NO_UINT64_MACROS
#ifdef __GNUC__
#define NO_UINT64_MACROS
    __attribute__((alias("scan_varint")));
#define NO_UINT64_MACROS
#else
#define NO_UINT64_MACROS
{
#define NO_UINT64_MACROS
  return scan_varint(dest, len, l);
#define NO_UINT64_MACROS
}
#define NO_UINT64_MACROS
#endif
