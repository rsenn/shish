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

      if(sep)
        tree_catseparator(&next, sep, depth);
      else
        stralloc_cats(&next, (node->ncmd.bgnd ? " & " : "; "));
    }

    if(0 && i && !sep) {

      add_len = byte_chr(next.s, next.len, '\n');
      stralloc_nul(sa);

      line_start = byte_rchr(sa->s, sa->len, '\n');

      line_len = sa->len - line_start;

      if(node->next && scan_whitenskip(&sa->s[line_start], line_len) < line_len - line_start) {
        if(line_len + add_len >= tree_columnwrap) {
          tree_catseparator(sa, "\\\n", depth);
        }
      }
    }

    stralloc_cat(sa, &next);
    i++;
  } while((node = node->next));

  stralloc_free(&next);
}
