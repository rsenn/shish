#include "../stralloc.h"
#undef stralloc_zero

void
stralloc_zero(stralloc* sa) {
  sa->len = 0;
}
