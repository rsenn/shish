#include "../tree.h"

const char* tree_separator = "  ";

/* ----------------------------------------------------------------------- */
void
tree_catseparator(stralloc* sa, const char* sep, int depth) {
  size_t i;

  for(i = 0; sep[i]; i++) {
    char c = sep[i];

   /* if(c == '\n' && depth < 0)
      c = ' ';*/

    stralloc_catc(sa, c);

    if(c == '\n') {
  int count;
      if(depth > 0)
        for(count = 0; count < depth; count++) stralloc_cats(sa, tree_separator);
    }
  }
}
