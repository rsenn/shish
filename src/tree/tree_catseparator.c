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

    /* a here-doc body starts on the line after its operator */
    if(c == '\n' && tree_here.len) {
      stralloc_cat(sa, &tree_here);
      tree_here.len = 0;
    }

    if(c == '\n') {
      int count;

      if(depth > 0)
        for(count = 0; count < depth; count++)
          stralloc_cats(sa, tree_separator);
    }
  }
}
