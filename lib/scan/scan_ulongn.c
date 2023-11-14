#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "../scan.h"

#if LONG_MAX == 2147483647 || defined(__i386__) || POINTER_SIZE == 4
#define IS_32BIT 1
#define WORD_SIZE 4
#elif LONG_MAX == 9223372036854775807L || defined(__x86_64__) || POINTER_SIZE == 8
#define IS_64BIT 1
#define WORD_SIZE 8
#endif

size_t
scan_ulongn(const char* src, size_t n, unsigned long int* dest) {
  const char* tmp = src;
  unsigned long int l = 0;
  unsigned char c;
/* Since the conditions can be computed at compile time, the compiler
 * should only emit code for one of the implementations, depending on
 * which architecture the code is compiled for. */
#ifdef HAVE_UINT128
  if(sizeof(unsigned long) == sizeof(uint64) && sizeof(unsigned long) < sizeof(__uint128_t)) {
    /* implementation for 64-bit platforms with gcc */
    for(; n-- > 0 && (c = (unsigned char)(*tmp - '0')) < 10; ++tmp) {
      __uint128_t L = (__uint128_t)l * 10 + c;

      if((L >> ((sizeof(L) - sizeof(l)) * 8)))
        break;
      l = (unsigned long)L;
    }
    *dest = l;
    return (size_t)(tmp - src);
  } else
#endif

#ifndef WORD_SIZE
      if(sizeof(unsigned long) < sizeof(uint64)) {
#endif
#if !defined(WORD_SIZE) || defined(IS_32BIT)
    /* implementation for 32-bit platforms */
    for(; n-- > 0 && (c = (unsigned char)(*tmp - '0')) < 10; ++tmp) {
      uint64 L = (uint64)l * 10 + c;

      if((unsigned long)L != L)
        break;
      l = (unsigned long)L;
    }
    *dest = l;
    return (size_t)(tmp - src);
#endif
#ifndef WORD_SIZE
  } else {
#endif
#if !defined(WORD_SIZE) || defined(IS_64BIT)
    /* implementation for 64-bit platforms without gcc */
    while(n-- > 0 && (c = (unsigned char)(*tmp - '0')) < 10) {
      unsigned long int n;
      /* we want to do: l=l*10+c
       * but we need to check for integer overflow.
       * to check whether l*10 overflows, we could do
       *   if((l*10)/10 != l)
       * however, multiplication and division are expensive.
       * so instead of *10 we do(l<<3) (i.e. *8) + (l<<1) (i.e. *2)
       * and check for overflow on all the intermediate steps */
      n = l << 3;

      if((n >> 3) != l)
        break;

      if(n + (l << 1) + c < n)
        break;
      l = n + (l << 1) + c;
      ++tmp;
    }

    if(tmp - src)
      *dest = l;
    return (size_t)(tmp - src);
#endif
#ifndef WORD_SIZE
  }
#endif
}
