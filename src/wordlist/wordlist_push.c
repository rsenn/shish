#include "../wordlist.h"
#include "../../lib/alloc.h"
#include "../../lib/byte.h"

/* freeze the open field as v[n]; an empty field with no quote or split bit is entered as
 * "" and marked pending, wordlist_settle() drops it unless a sibling claims it first
 * ----------------------------------------------------------------------- */
void
wordlist_push(wordlist* wl) {
  stralloc* c = wl->cur;
  int empty = c->len == 0 && !(wl->state & (X_QUOTED | X_NOSPLIT | X_SPLIT));
  char* f = (char*)"";

  if(wl->n + 2 > wl->a) {
    size_t na = wl->a * 2;
    char** nv = wl->v == wl->inl ? alloc(na * sizeof(char*)) : alloc_re(wl->v, na * sizeof(char*));

    if(nv == NULL) {
      wl->oom = 1;
      c->len = 0;
      return;
    }

    if(wl->v == wl->inl)
      byte_copy(nv, wl->n * sizeof(char*), wl->inl);

    wl->v = nv;
    wl->a = na;
  }

  if(c->len && (f = arena_strndup(wl->ar, c->s, c->len)) == NULL) {
    wl->oom = 1;
    c->len = 0;
    return;
  }

  wl->v[wl->n++] = f;
  wl->v[wl->n] = NULL;
  wl->pend = empty;
  c->len = 0;
}
