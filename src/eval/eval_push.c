#include "../../lib/byte.h"
#include "../fd.h"
#include "../eval.h"
#include "../fdstack.h"
#include "../sh.h"
#include "../vartab.h"

/*struct eval *eval = NULL;*/

void
eval_push(struct eval* e, int flags) {
  byte_zero(e, sizeof(struct eval));
  e->flags = flags | (sh->opts.xtrace ? E_PRINT : 0);
  e->parent = sh->eval;

  /* remember stack locations in current nesting level */
  e->fdstack = fdstack;
  e->varstack = varstack;

  sh->eval = e;
}
