#include "../wordlist.h"
#include "../expand.h"
#include "../parse.h"
#include "../../lib/str.h"

/* end of a word: finish the open field the way its state bits ask, keep or drop it
 * ----------------------------------------------------------------------- */
int
wordlist_close(wordlist* wl) {
  size_t mark = wl->mark;

  if(wl->ar == NULL) {
    stralloc_nul(wl->cur);
    return 0;
  }

  /* a field closed by the last chunk's own splitting is finished again by the bits it
     collected: "\\\\ " is unescaped, "f? " is globbed. Take it back out of v for that. */
  if(wl->closed && !wl->pend && wl->n && (wl->state & (X_GLOB | X_GLOBRES | X_LITERAL))) {
    char* f = wl->v[--wl->n];

    wl->v[wl->n] = NULL;
    stralloc_copyb(wl->cur, f, str_len(f));
    wl->closed = 0;
  }

  if(wl->has && !wl->closed) {
    stralloc_nul(wl->cur);

    if(wl->state & X_GLOB)
      wordlist_glob(wl, wl->state & ~X_GLOB);
    else if(wl->state & X_GLOBRES)
      wordlist_glob(wl, wl->state);
    else if((wl->state & X_LITERAL) && !(wl->state & X_UNESCAPED))
      expand_unescape(wl->cur, parse_isesc);

    wordlist_push(wl);
  }

  wordlist_settle(wl);
  wl->has = wl->closed = 0;
  wl->mark = wl->n;
  return (int)(wl->n - mark);
}
