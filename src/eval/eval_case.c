#include "../eval.h"
#include "../expand.h"
#include "../../lib/shell.h"
#include "../../lib/stralloc.h"
#include "../tree.h"

/* evaluate case conditional construct (3.9.4.3)
 * ----------------------------------------------------------------------- */
int
eval_case(struct eval* e, struct ncase* ncase) {
  union node* node;
  union node* pat;
  int ret = 0;
  stralloc word;
  stralloc pattern;
  stralloc_init(&word);
  stralloc_init(&pattern);

  if(ncase->word)
    expand_catsa(ncase->word, &word, X_NOSPLIT);

  stralloc_nul(&word);

  for(node = ncase->list; node; node = node->next) {
    for(pat = node->ncasenode.pats; pat; pat = pat->next) {
      expand_catsa(pat, &pattern, X_NOSPLIT);
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
