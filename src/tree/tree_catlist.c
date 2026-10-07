#include "../tree.h"
#include "../../lib/byte.h"
#include "../../lib/scan.h"

int tree_columnwrap = -1;

void
tree_catlist(union node* node, stralloc* sa, const char* sep) {
  tree_here_enter();
  tree_catlist_n(node, sa, sep, 0);
  tree_here_leave(sa);
  stralloc_nul(sa);
}

/* print (sub)tree(list) to a stralloc
 * ----------------------------------------------------------------------- */
void
tree_catlist_n(union node* node, stralloc* sa, const char* sep, int depth) {
  size_t i = 0;
  stralloc next;

  /* an empty case statement ("case x in esac") has a NULL
     ncase.list -- N_CASE's own tree_cat_n() branch passes it
     straight through with no NULL check of its own, so this list
     ends up genuinely empty rather than a caller bug
     (case-with-empty-body-crashes-tree-stringifier) */
  if(!node)
    return;

  stralloc_init(&next);

  do {
    size_t line_start, add_len, line_len;

    stralloc_zero(&next);
    tree_cat_n(node, &next, depth);
    stralloc_nul(&next);

    /* "a & b": the "&" ends the statement, so it replaces a ";" separator */
    if(tree_isbgnd(node))
      stralloc_cats(&next, " &");

    if(node->next) {
      if(sep /*&& *sep*/)
        tree_catseparator(&next, tree_isbgnd(node) && *sep == ';' ? sep + 1 : sep, depth);
      else
        stralloc_cats(&next, tree_isbgnd(node) ? " " : "; ");
    }

    stralloc_cat(sa, &next);
    i++;
  } while((node = node->next));

  stralloc_free(&next);
}
