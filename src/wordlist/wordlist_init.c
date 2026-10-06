#include "../wordlist.h"
#include "../../lib/byte.h"
#include <stddef.h>

/* field mode: closed fields go to ar, splitting happens at the characters of ifs
 *
 *   wordlist*    wl    list to set up
 *   arena*       ar    where closed fields are frozen
 *   const char*  ifs   splitting characters, NULL = no splitting
 * ----------------------------------------------------------------------- */
void
wordlist_init(wordlist* wl, arena* ar, const char* ifs) {
  byte_zero(wl, sizeof(*wl));
  wl->ar = ar;
  wl->ifs = ifs;
  wl->cur = wordlist_pool_get();

  if(wl->cur == NULL)
    wl->cur = &wl->own;

  wl->cur->len = 0;
  wl->v = wl->inl;
  wl->a = WORDLIST_INLINE;
}
