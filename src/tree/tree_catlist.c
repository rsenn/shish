#include "../tree.h"
#include "../../lib/byte.h"
#include "../../lib/scan.h"

int tree_columnwrap = -1;

void
tree_catlist(union node* node, stralloc* sa, const char* sep) {
  tree_catlist_n(node, sa, sep, 0);
}

/* print (sub)tree(list) to a stralloc
 * ----------------------------------------------------------------------- */
void
tree_catlist_n(union node* node, stralloc* sa, const char* sep, int depth) {
  ssize_t line_len, line_start;
  stralloc next;
  stralloc_init(&next);

  do {
    stralloc_zero(&next);
    tree_cat_n(node, &next, depth);

    if(node->list.next || (node->id == N_SIMPLECMD && node->ncmd.bgnd)) {

      if(sep)
        tree_catseparator(&next, sep, depth);
      else
        stralloc_cats(&next, (node->ncmd.bgnd ? " & " : "; "));
    }

    line_len = byte_chr(next.s, next.len, '\n');

    stralloc_readyplus(sa, next.len + next.len >> 1);

    if((line_start = byte_rchr(sa->s, sa->len, '\n')) == sa->len)
      line_start = 0;

    // if(scan_whitenskip(&sa->s[line_start], line_len) < line_len)
    {
      if(line_start + line_len >= tree_columnwrap) {
        tree_catseparator(sa, "\\\n", depth);
      }
    }

    stralloc_cat(sa, &next);

  } while((node = node->list.next));

  stralloc_free(&next);
}
