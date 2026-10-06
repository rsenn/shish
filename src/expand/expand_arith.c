#include "../expand.h"
#include "../../lib/fmt.h"
#include "../tree.h"
#include "../sh.h"

/* expand an arithmetic expression
 * ----------------------------------------------------------------------- */
void
expand_arith(struct nargarith* arith, wordlist* wl, int flags) {
  union node* expr = arith->tree;
  int64 ret = -1;
  size_t len;
  char buf[FMT_LONG];

  if(!expand_arith_expr(expr, &ret)) {
    len = fmt_longlong(buf, ret);
    wordlist_cat(wl, buf, len, flags);
  } else {
    /* an arithmetic expansion error ends a non-interactive shell, like any expansion error */
    sh_error("arithmetic syntax error");

    if(!sh_interactive)
      sh_exit(1);

    expand_error = 1;
  }
}
