#include "../tree.h"
#include "../buffer.h"

void
tree_print(union node* node, buffer* b) {
  stralloc sa;
  stralloc_init(&sa);
  tree_cat(node, &sa);
  buffer_putsa(b, &sa);
  stralloc_free(&sa);
}
