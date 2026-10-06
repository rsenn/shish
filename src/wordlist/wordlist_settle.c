#include "../wordlist.h"

void
wordlist_settle(wordlist* wl) {
  if(wl->pend) {
    wl->v[--wl->n] = NULL;
    wl->pend = 0;
  }
}
