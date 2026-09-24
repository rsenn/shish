#include "dfa_prog.h"

int
dfa_test(struct dfa* d, const char* s, size_t n) {
  struct dfa_span m;

  return d->has_backref ? dfa_bt_run(d, s, n, 0, 0, 0, &m, 0, 0)
                         : dfa_run(d, s, n, 0, 0, 0, &m, 0, 0);
}
