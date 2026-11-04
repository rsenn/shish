#include "walk_internal.h"

void
walk_keep(struct walk* w) {
  if(w->nf)
    w->f[w->nf - 1].kept = 1;
}
