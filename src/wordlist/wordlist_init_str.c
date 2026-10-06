#include "../wordlist.h"
#include "../../lib/byte.h"

/* string mode: one field, never split, never frozen
 * ----------------------------------------------------------------------- */
void
wordlist_init_str(wordlist* wl, stralloc* out) {
  byte_zero(wl, sizeof(*wl));
  wl->cur = out;
  wl->has = 1;
}
