#include "eval.h"
#include "expand.h"
#include "shell.h"
#include "stralloc.h"
#include "tree.h"

/* evaluate case conditional construct (3.9.4.3)
 * ----------------------------------------------------------------------- */
int
eval_case(struct eval* e, struct ncase* ncase) {
  union node* node;
  union node* pat;
  int ret = 0;
    struct expand x = EXPAND_INIT(0, 0, X_NOSPLIT);

  stralloc word;
  stralloc pattern;
  stralloc_init(&word);
  stralloc_init(&pattern);

  if(ncase->word)
    expand_appendsa( ncase->word, &word);

  stralloc_nul(&word);

  for(node = ncase->list; node; node = node->list.next) {
    for(pat = node->ncasenode.pats; pat; pat = pat->list.next) {
      expand_appendsa(pat, &pattern);
      stralloc_nul(&pattern);

      if(shell_fnmatch(pattern.s, pattern.len, word.s, word.len, SH_FNM_PERIOD) == 0) {
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
