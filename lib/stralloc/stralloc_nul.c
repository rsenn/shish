#include "../stralloc.h"

int
stralloc_nul(stralloc* sa) {
  if(!stralloc_readyplus(sa, 1))
    return 0;
  /*
    if(sa->len > 0 && sa->s[sa->len - 1] == '\0')
      return 0;
  */
  sa->s[sa->len] = '\0';
  return 1;
}
