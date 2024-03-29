#include "../expand.h"

char*
expand_tostr(union node* node, int flags) {
  stralloc sa;
  stralloc_init(&sa);

  expand_str(node, &sa, flags);

  stralloc_nul(&sa);

  return sa.s;
}
