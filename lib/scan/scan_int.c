#include "../scan.h"

static const unsigned int maxint = ((unsigned int)-1) >> 1;

size_t
scan_int(const char* src, int* dest) {
  const char* tmp;
  int l;
  unsigned char c;
  unsigned int neg;
  int ok;
  tmp = src;
  l = 0;
  ok = 0;
  neg = 0;

  switch(*tmp) {
    case '-': {
      neg = 1;
    }
    case '+': {
      ++tmp;
      break;
    }
  }

  while((c = (unsigned char)(*tmp - '0')) < 10) {
    unsigned int n;
    /* we want to do: l=l*10+c
     * but we need to check for integer overflow.
     * to check whether l*10 overflows, we could do
     *   if((l*10)/10 != l)
     * however, multiplication and division are expensive.
     * so instead of *10 we do (l<<3) (i.e. *8) + (l<<1) (i.e. *2)
     * and check for overflow on all the intermediate steps */
    n = (unsigned int)l << 3;

    if((n >> 3) != (unsigned int)l)
      break;

    if(n + ((unsigned int)l << 1) < n)
      break;
    n += (unsigned int)l << 1;

    if(n + c < n)
      break;
    n += c;

    if(n > maxint + neg)
      break;
    l = (int)n;
    ++tmp;
    ok = 1;
  }

  if(!ok)
    return 0;
  *dest = (neg ? -l : l);
  return (size_t)(tmp - src);
}
