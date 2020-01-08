#include "../uint64.h"
#include "../scan.h"

size_t
scan_varint(const char* in, size_t len, uint64* n) {
  size_t i;
  uint64 l;
  if(len == 0)
    return 0;
  l = 0;
  for(l = 0, i = 0; i < len; ++i) {
    l += (in[i] & 0x7f) << (i * 7);
    if(!(in[i] & 0x80)) {
      *n = l;
      return i + 1;
    }
  }
  return 0;
}

size_t scan_pb_type0_int(const char* dest, size_t len, uint64* l)
#ifdef __GNUC__
    __attribute__((alias("scan_varint")));
#else
{
  return scan_varint(dest, len, l);
}
#endif
