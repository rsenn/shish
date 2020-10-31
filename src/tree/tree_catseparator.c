#include "../tree.h"

const char* tree_separator = "  ";

/* ----------------------------------------------------------------------- */
void
tree_catseparator(stralloc* sa, const char* sep, int depth) {
  size_t i;
  int count;

  for(i = 0; sep[i]; i++) {
    char c = sep[i];
    stralloc_catc(sa, c);

    if(c == '\n') {
      for(count = 0; count < depth; count++) stralloc_cats(sa, tree_separator);
    }
  }
}