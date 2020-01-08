#define NO_UINT64_MACROS
#include "../fmt.h"
#include "../uint64.h"

size_t
fmt_longlong(char* dest, int64 i) {
  if(i < 0) {
    if(dest)
      *dest++ = '-';
    return fmt_ulonglong(dest, (uint64)-i) + 1;
  } else
    return fmt_ulonglong(dest, (uint64)i);
}
