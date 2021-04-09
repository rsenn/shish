#include "../tree.h"

const char*
node2str(const union node n) {
  stralloc sa;
  stralloc_init(&sa);

  tree_cat((union node*)&n, &sa);
  stralloc_nul(&sa);
  return stralloc_take(&sa, 0, 0);
}
