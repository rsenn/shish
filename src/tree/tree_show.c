#include "../tree.h"
#include "../fdtable.h"

void
tree_show(union node* node) {
  tree_print(node, fd_err->w);
  buffer_putnlflush(fd_err->w);
}
