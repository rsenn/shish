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
scan_octal(const char* src, uint64* dest) {
#define NO_UINT64_MACROS
  /* make a copy of src so we can return the number of bytes we progressed */
#define NO_UINT64_MACROS
  const char* tmp = src;
#define NO_UINT64_MACROS
  uint64 l = 0;
#define NO_UINT64_MACROS
  unsigned char c;
#define NO_UINT64_MACROS
  /* *tmp - '0' can be negative, but casting to unsigned char makes
#define NO_UINT64_MACROS
   * those cases positive and large; that means we only need one
#define NO_UINT64_MACROS
   * comparison.  This trick is no longer needed on modern compilers,
#define NO_UINT64_MACROS
   * but we also want to produce good code on old compilers :) */
#define NO_UINT64_MACROS
  while((c = (unsigned char)(*tmp - '0')) < 8) {
#define NO_UINT64_MACROS
    /* overflow check; for each digit we multiply by 8 and then add the
#define NO_UINT64_MACROS
     * digit; 0-7 needs 3 bits of storage, so we need to check if the
#define NO_UINT64_MACROS
     * uppermost 3 bits of l are empty. Do it by shifting to the right */
#define NO_UINT64_MACROS
    if(l >> (sizeof(l) * 8 - 3))
#define NO_UINT64_MACROS
      break;
#define NO_UINT64_MACROS
    l = l * 8 + c;
#define NO_UINT64_MACROS
    ++tmp;
#define NO_UINT64_MACROS
  }
#define NO_UINT64_MACROS
  *dest = l;
#define NO_UINT64_MACROS
  return (size_t)(tmp - src);
#define NO_UINT64_MACROS
}
