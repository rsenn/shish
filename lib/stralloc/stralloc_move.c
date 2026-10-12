#include "../byte.h"
#include "../stralloc.h"
#include <stdlib.h>

/* stralloc_move transfers ownership from one stralloc to another,
   freeing the destination first. */
void
stralloc_move(stralloc* to, stralloc* from) {
  if(to->a == 0)
    to->s = NULL;

  stralloc_free(to);
  to->s = stralloc_take(from, &to->len, &to->a);
}
