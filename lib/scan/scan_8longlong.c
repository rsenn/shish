#include "../uint64.h"
#include "../scan.h"

size_t
scan_8longlong(const char* src, uint64* dest) {
  return scan_8longlongn(src, (size_t)-1, dest);
}
