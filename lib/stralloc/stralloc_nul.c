#include "../stralloc.h"

int
stralloc_nul(stralloc* sa) {
  if(!stralloc_readyplus(sa, 1))
    return 0;

  sa->s[sa->len] = '\0';
  return 1;
}
