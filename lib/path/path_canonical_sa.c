#include "../path_internal.h"

int
path_canonical_sa(stralloc* sa) {
  sa->len = path_collapse(sa->s, sa->len);
  stralloc_nul(sa);
  return 1;
}
