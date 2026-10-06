#include "../wordlist.h"
#include "../../lib/alloc.h"

/* wordlist_init_str() lists have no arena, no pool buffer and no v: nothing to give back
 * ----------------------------------------------------------------------- */
void
wordlist_free(wordlist* wl) {
  if(wl->ar == NULL)
    return;

  if(wl->v != wl->inl)
    alloc_free(wl->v);

  wl->v = wl->inl;

  if(wl->cur == &wl->own)
    stralloc_free(&wl->own);
  else
    wordlist_pool_put(wl->cur);

  wl->cur = NULL;
}
