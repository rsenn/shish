#include "../tree.h"
#include "../../lib/stralloc.h"

char*
tree_string(union node* node) {
  stralloc sa;
  stralloc_init(&sa);
  if(node->id == N_ARG)
    tree_catlist(node, &sa, " ");
  else
    tree_cat(node, &sa);
  stralloc_nul(&sa);
  return sa.s;
}
