#include "../tree.h"
#include "../../lib/byte.h"
#include "../../lib/scan.h"

int tree_columnwrap = -1;

void
tree_catlist(union node* node, stralloc* sa, const char* sep) {
  tree_catlist_n(node, sa, sep, 0);
  stralloc_nul(sa);
}

/* print (sub)tree(list) to a stralloc
 * ----------------------------------------------------------------------- */
void
tree_catlist_n(union node* node, stralloc* sa, const char* sep, int depth) {
  size_t i = 0;
  stralloc next;
  stralloc_init(&next);

  do {
    size_t line_start, add_len, line_len;

    stralloc_zero(&next);
    tree_cat_n(node, &next, depth);
    stralloc_nul(&next);

    if(node->next || (node->id == N_SIMPLECMD && node->ncmd.bgnd)) {

      if(sep /*&& *sep*/)
        tree_catseparator(&next, sep, depth);
      else
        stralloc_cats(&next, (node->ncmd.bgnd ? " & " : "; "));
    }

    stralloc_cat(sa, &next);
    i++;
  } while((node = node->next));

  stralloc_free(&next);
}
