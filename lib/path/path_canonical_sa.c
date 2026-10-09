#include "../path_internal.h"

/* normalizes sa in place (see path_collapse()); "" becomes "." */
int
path_canonical_sa(stralloc* sa) {
  sa->len = path_collapse(sa->s, sa->len);

  if(sa->len == 0 && !stralloc_catc(sa, '.'))
    return 0;

  return stralloc_nul(sa);
}
