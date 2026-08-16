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

  /* var->sa.s must never be NULL: var_bsearch(), var_print(), and
     var_export() all read var->len bytes from it unconditionally.
     Store the name now and mark V_UNSET so a variable that only ever
     went through var_chflg() (e.g. "export FOO") still behaves like a
     real, if valueless, variable. */
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
