#include "../byte.h"
#include "../stralloc.h"

/* stralloc_remove removes a range of len bytes starting at pos. */
size_t
stralloc_remove(stralloc* sa, size_t pos, size_t len) {
  if(pos + 1 > sa->len)
    return -1;

  if(pos + len >= sa->len) {
    len = sa->len - pos;
    sa->len = pos;
    return len;
  }

  byte_copy(&sa->s[pos], sa->len - (pos + len), &sa->s[pos + len]);
  sa->len -= len;
  return len;
}
