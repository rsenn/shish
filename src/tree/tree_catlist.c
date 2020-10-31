#include "../tree.h"

void
tree_catlist(union node* node, stralloc* sa, const char* sep) {
  tree_catlist_n(node, sa, sep, 0);
}
/* print (sub)tree(list) to a stralloc
 * ----------------------------------------------------------------------- */
void
tree_catlist_n(union node* node, stralloc* sa, const char* sep, int depth) {
  do {
    tree_cat_n(node, sa, depth);

    if(node->list.next || (node->id == N_SIMPLECMD && node->ncmd.bgnd)) {

      if(sep)
        tree_catseparator(sa, sep, depth);
      else
        stralloc_cats(sa, (node->ncmd.bgnd ? " & " : "; "));
    }

  } while((node = node->list.next));
}
