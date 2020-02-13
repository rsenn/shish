#include "../byte.h"
#include "../stralloc.h"

int
stralloc_insertb(stralloc* sa, const char* s, size_t pos, size_t n) {
  if(pos >= sa->len)
    return stralloc_catb(sa, s, n);
  if(!stralloc_readyplus(sa, n))
    return 0;
  byte_copyr(&sa->s[pos + n], sa->len - pos, &sa->s[pos]);
  byte_copy(&sa->s[pos], n, s);
  sa->len += n;
  return 1;
}
