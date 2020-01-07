#include "tree.h"

/* count nodes in a flat list */
unsigned int
tree_count(union node* node) {
  unsigned int n;

  for(n = 0; node; node = node->list.next) n++;

  return n;
}
