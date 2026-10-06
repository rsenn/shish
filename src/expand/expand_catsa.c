#include "../expand.h"
#include "../tree.h"

/* expand one N_ARG nodes to a stralloc (appending)
 * ----------------------------------------------------------------------- */
void
expand_catsa(union node* node, stralloc* sa, int flags) {
  wordlist wl;

  wordlist_init_str(&wl, sa);
  expand_arg(node, &wl, flags | X_NOSPLIT);
  wordlist_close(&wl);
}
