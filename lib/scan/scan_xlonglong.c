#define NO_UINT64_MACROS
#include "../uint64.h"
#define NO_UINT64_MACROS
#include "../uint64.h"
#define NO_UINT64_MACROS
#include "../scan.h"
#define NO_UINT64_MACROS

#define NO_UINT64_MACROS
size_t
#define NO_UINT64_MACROS
scan_xlonglong(const char* src, uint64* dest) {
#define NO_UINT64_MACROS
  const char* tmp = src;
#define NO_UINT64_MACROS
  int64 l = 0;
#define NO_UINT64_MACROS
  unsigned char c;
#define NO_UINT64_MACROS
  while((c = scan_fromhex(*tmp)) < 16) {
#define NO_UINT64_MACROS
    l = (l << 4) + c;
#define NO_UINT64_MACROS
    ++tmp;
#define NO_UINT64_MACROS
  }
#define NO_UINT64_MACROS
  *dest = l;
#define NO_UINT64_MACROS
  return tmp - src;
#define NO_UINT64_MACROS
}
