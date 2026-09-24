#include "dfa_prog.h"

int
dfa_search(struct dfa* d, const char* s, size_t n, size_t from, struct dfa_span* m) {
  int notbol = (d->flags & DFA_NOTBOL) != 0;

  return d->has_backref ? dfa_bt_run(d, s, n, from, 0, notbol, m, 0, 0)
                         : dfa_run(d, s, n, from, 0, notbol, m, 0, 0);
}
