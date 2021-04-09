#include "../tree.h"
#include "../../lib/buffer.h"

void
tree_printlist(union node* node, const char* sep, buffer* b) {
  stralloc sa;
  stralloc_init(&sa);
  tree_catlist(node, &sa, sep);
  buffer_putsa(b, &sa);
  stralloc_free(&sa);
}
