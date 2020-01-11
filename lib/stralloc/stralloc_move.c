#include "../byte.h"
#include "../stralloc.h"
#include <stdlib.h>

void
stralloc_move(stralloc* to, stralloc* from) {
  if(to->a == 0)
    to->s = NULL;
  stralloc_free(to);
  to->s = from->s;
  to->a = from->a;
  to->len = from->len;
}
