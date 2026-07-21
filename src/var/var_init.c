#include "../../lib/byte.h"
#include "../var.h"

struct var*
var_init(const char* v, struct var* var, struct search* context) {
  var->bnext = NULL;
  var->gnext = NULL;
  var->blink = NULL;
  var->glink = NULL;
  stralloc_init(&var->sa);
  var->offset = var->len = context->len;

  /* var->sa.s must never be NULL: var_bsearch() (var lookup) and
     var_print()/var_export() all read var->len bytes from it
     unconditionally, with no separate place to keep the name if it
     isn't in there. A variable can go through var_chflg() (e.g.
     "export FOO") with no value ever assigned -- store its name now
     and mark it V_UNSET, so it behaves like a real (if valueless)
     variable everywhere else instead of leaving sa.s dangling until
     whatever the first real assignment turns out to be. */
  stralloc_copyb(&var->sa, v, context->len);
  stralloc_nul(&var->sa);
  var->flags = V_UNSET;

  var->lexhash = context->lexhash;
  var->rndhash = context->rndhash;
  var->parent = NULL;
  var->child = NULL;
  var->table = NULL;

  return var;
}
