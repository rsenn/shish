#include "../byte.h"
#include "../stralloc.h"

size_t
stralloc_remove(stralloc* sa, size_t pos, size_t n) {
  if(pos + 1 > sa->len)
    return -1;

  if(pos + n >= sa->len) {
    n = sa->len - pos;
    sa->len = pos;
    return n;
  }
  byte_copy(&sa->s[pos], sa->len - (pos + n), &sa->s[pos + n]);
  sa->len -= n;
  return n;
}
