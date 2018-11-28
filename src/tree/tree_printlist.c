#include "tree.h"

/* print (sub)tree(list) to a stralloc
 * ----------------------------------------------------------------------- */
void
tree_printlist(union node* node, stralloc* sa, const char* sep) {
  do {
    tree_print(node, sa);

    if(node->list.next || node->ncmd.bgnd) {

      stralloc_cats(sa, sep ? sep : (node->ncmd.bgnd ? " & " : "; "));
    }

  } while((node = node->list.next));
}
