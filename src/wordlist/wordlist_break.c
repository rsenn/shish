#include "../wordlist.h"

/* end the open field as it is: no glob, no unescape, an empty unquoted one is dropped
 * ----------------------------------------------------------------------- */
void
wordlist_break(wordlist* wl) {
  if(wl->ar == NULL || !wl->has)
    return;

  if(!wl->closed)
    wordlist_push(wl);

  wordlist_settle(wl);
  wl->has = wl->closed = 0;
}
