#include "stralloc.h"

int stralloc_catc(stralloc *sa,const unsigned char c) {
  if (stralloc_ready(sa,sa->len+1)) {
    sa->s[sa->len]=c;
    sa->len++;
    return 1;
  }
  return 0;
}
