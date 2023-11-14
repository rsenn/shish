#include "../eval.h"
#include "../expand.h"
#include "../../lib/path.h"
#include "../../lib/stralloc.h"
#include "../tree.h"
#include "../fdtable.h"

/* evaluate case conditional construct (3.9.4.3)
 * ----------------------------------------------------------------------- */
int
eval_case(struct eval* e, struct ncase* ncase) {
  union node *node, *pat;
  int ret = 0;

  stralloc word;
  stralloc pattern;
  stralloc_init(&word);
  stralloc_init(&pattern);

  if(ncase->word)
    expand_catsa(ncase->word, &word, X_NOSPLIT);

  stralloc_nul(&word);

  if(e->flags & E_PRINT) {
    eval_print_prefix(e, fd_err->w);

    buffer_puts(fd_err->w, "case ");
    tree_print(ncase->word, fd_err->w);
    buffer_puts(fd_err->w, " in");
    buffer_putnlflush(fd_err->w);
  }

  for(node = ncase->list; node; node = node->next) {
    for(pat = node->ncasenode.pats; pat; pat = pat->next) {
      expand_catsa(pat, &pattern, X_NOSPLIT);
      stralloc_nul(&pattern);

      if(path_fnmatch(pattern.s, pattern.len, word.s, word.len, SH_FNM_PERIOD) == 0) {
        ret = eval_tree(e, node->ncasenode.cmds, E_LIST);
        goto end;
      }

      stralloc_zero(&pattern);
    }
  }

end:
  stralloc_free(&pattern);
  stralloc_free(&word);

  return ret;
}
