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
  ssize_t len, line_start;
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

    if(byte_chr(next.s, next.len, '\n') == next.len) {
      if(sa->s && sa->len) {
        line_start = byte_rchr(sa->s, sa->len, '\n');
        if(line_start == sa->len)
          line_start = 0;
        len = (ssize_t)sa->len - line_start;

        if(scan_whitenskip(&sa->s[line_start], len) < len) {

          if((len - line_start + next.len) >= tree_columnwrap)
            tree_catseparator(sa, "\\\n", depth);
        }
      }
    }

    stralloc_cat(sa, &next);

  } while((node = node->list.next));

  stralloc_free(&next);
}
