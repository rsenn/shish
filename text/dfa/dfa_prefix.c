#include "dfa_prog.h"

long
dfa_prefix(struct dfa* d, const char* s, size_t n) {
  struct dfa_span m;
  int ok = d->has_backref ? dfa_bt_run(d, s, n, 0, 1, 0, &m, 0, 0)
                           : dfa_run(d, s, n, 0, 1, 0, &m, 0, 0);

  return ok ? (long)(m.end - m.start) : -1;
}
