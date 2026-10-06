#include "../expand.h"
#include "../tree.h"

/* expand one N_ARG node to a stralloc (stralloc is overwritten!!!)
 * ----------------------------------------------------------------------- */
void
expand_copysa(union node* node, stralloc* sa, int flags) {
  wordlist wl;

  stralloc_init(sa);
  wordlist_init_str(&wl, sa);

  /* string mode routes every chunk through the non-splitting branch of
     wordlist_cat(), which unescapes each literal chunk as it's appended. */
  expand_arg(node, &wl, flags | X_NOSPLIT);
  wordlist_close(&wl);
}
