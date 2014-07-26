#include <byte.h>
#include <stralloc.h>

int stralloc_remove(stralloc* sa, unsigned long pos, unsigned long n) {
  if (pos + 1 > sa->len) return -1;
  if(pos + n >= sa->len) {
    n = sa->len - pos;
    sa->len = pos;
    return n;
  }
  byte_copy(&sa->s[pos], sa->len - (pos + n), &sa->s[pos + n]);
  sa->len -= n;
  return n;
}
