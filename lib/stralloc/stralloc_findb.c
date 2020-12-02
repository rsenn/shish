#include "../byte.h"
#include "../stralloc.h"
/*
  return index to first occurance of data,
  otherwise return sa->len
*/
size_t
stralloc_findb(const stralloc* sa, const void* what, size_t len) {
  size_t i, n, last;
  char* s;
  n = sa->len;
  if(n < len)
    return n;
  last = n - len;
  s = sa->s;
  for(i = 0; i <= last; i++) {
    if(byte_equal(s, len, what))
      return i;
    s++;
  }
  return n;
}
