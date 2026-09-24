#include "dfa_prog.h"
#include "../../lib/alloc.h"
#include "../../lib/byte.h"

void
dfa_free(struct dfa* d) {
  alloc_free(d->prog);
  alloc_free(d->sets);
  byte_zero(d, sizeof(*d));
}

size_t
dfa_groups(const struct dfa* d) {
  return d->ngroup;
}
