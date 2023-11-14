#include "../scan.h"

#if LONG_MAX == 2147483647 || defined(__i386__) || POINTER_SIZE == 4
#define IS_32BIT 1
#define WORD_SIZE 4
#elif LONG_MAX == 9223372036854775807L || defined(__x86_64__) || POINTER_SIZE == 8
#define IS_64BIT 1
#define WORD_SIZE 8
#endif

/* this is cut and paste from scan_ulong instead of calling scan_ulong
 * because this way scan_uint can abort scanning when the number would
 * not fit into an unsigned int (as opposed to not fitting in an
 * unsigned long) */

size_t
scan_uint(const char* src, unsigned int* dest) {
#ifndef WORD_SIZE
  if(sizeof(unsigned int) == sizeof(unsigned long)) {
#endif
#if !defined(WORD_SIZE) || defined(IS_32BIT)
    /* a good optimizing compiler should remove the else clause when not
     * needed */
    return scan_ulong(src, (unsigned long*)dest);
#endif
#ifndef WORD_SIZE
  } else if(sizeof(unsigned int) < sizeof(unsigned long)) {
#endif
#if !defined(WORD_SIZE) || defined(IS_64BIT)
    const char* cur;
    unsigned int l;

    for(cur = src, l = 0; *cur >= '0' && *cur <= '9'; ++cur) {
      unsigned long tmp = l * 10ul + *cur - '0';

      if((unsigned int)tmp != tmp)
        break;
      l = tmp;
    }

    if(cur > src)
      *dest = l;
    return (size_t)(cur - src);
#endif
#ifndef WORD_SIZE
  }
#endif
}
