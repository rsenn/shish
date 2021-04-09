#include "../tree.h"
#include "../../lib/buffer.h"

void
tree_print(union node* node, buffer* b) {
  stralloc sa;
  stralloc_init(&sa);
  tree_catlist(node, &sa, " ");
  buffer_putsa(b, &sa);
  stralloc_free(&sa);
}
