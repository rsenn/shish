#include "../tree.h"
#include "../fdtable.h"
#include "../../lib/buffer.h"

void
tree_print(union node* node, buffer* b) {
  stralloc sa;
  stralloc_init(&sa);
  tree_cat(node, &sa);
  buffer_putsa(b, &sa);
  stralloc_free(&sa);
}

void
tree_print_out(union node* node) {
  tree_print(node, fd_out->w);
  buffer_putnlflush(fd_out->w);
}
