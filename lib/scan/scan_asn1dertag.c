#include "../uint64.h"
#define NO_UINT64_MACROS
#define NO_UINT64_MACROS
#define NO_UINT64_MACROS
#include "../scan.h"
#define NO_UINT64_MACROS

#define NO_UINT64_MACROS
/* ASN.1 DER tag encoding parser.
#define NO_UINT64_MACROS
 * Write value as big endian series of bytes, each containing seven
#define NO_UINT64_MACROS
 * bits.  In each byte except the last, set the highest bit.
#define NO_UINT64_MACROS
 * Examples:
#define NO_UINT64_MACROS
 *   0 -> 0x00
#define NO_UINT64_MACROS
 *   5 -> 0x05
#define NO_UINT64_MACROS
 *   0xc2 -> 0x81 0x42 */
#define NO_UINT64_MACROS

#define NO_UINT64_MACROS
size_t
#define NO_UINT64_MACROS
scan_asn1dertag(const char* src, size_t len, uint64* length) {
#define NO_UINT64_MACROS
  size_t n;
#define NO_UINT64_MACROS
  uint64 l = 0;
#define NO_UINT64_MACROS
  if(len == 0 || (unsigned char)src[0] == 0x80)
#define NO_UINT64_MACROS
    return 0; /* catch non-minimal encoding */
#define NO_UINT64_MACROS
  for(n = 0; n < len; ++n) {
#define NO_UINT64_MACROS
    /* make sure we can represent the stored number in l */
#define NO_UINT64_MACROS
    if(l >> (sizeof(l) * 8 - 7))
#define NO_UINT64_MACROS
      return 0; /* catch integer overflow */
#define NO_UINT64_MACROS
    l = (l << 7) | (src[n] & 0x7f);
#define NO_UINT64_MACROS
    /* if high bit not set, this is the last byte in the sequence */
#define NO_UINT64_MACROS
    if(!(src[n] & 0x80)) {
#define NO_UINT64_MACROS
      *length = l;
#define NO_UINT64_MACROS
      return n + 1;
#define NO_UINT64_MACROS
    }
#define NO_UINT64_MACROS
  }
#define NO_UINT64_MACROS
  return 0;
#define NO_UINT64_MACROS
}
