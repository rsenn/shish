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

      /* unlike pathname expansion (globbing), case-statement pattern
         matching has no "leading dot must be matched explicitly"
         rule -- POSIX 2.9.4.3 draws case's matching straight from
         2.13.1 (pattern matching notation) rather than 2.13.3
         (pathname expansion, where FNM_PERIOD-style hiding applies),
         so a scrutinee value that happens to start with "." (e.g.
         ${var} holding a literal "." or "..") must still match a
         plain "*"/"?" pattern like any other character. Passing
         SH_FNM_PERIOD here made every case statement whose word
         started with "." fail to match any pattern that didn't
         itself start with a literal "." -- including "*", the
         universal fallback (case-pattern-bracket-quote-stripping's
         repro looked like a bracket/quoting bug but reduces to this
         same root cause: "*") never matched a leading-dot word
         either). */
      if(path_fnmatch(pattern.s, pattern.len, word.s, word.len, 0) == 0) {
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
