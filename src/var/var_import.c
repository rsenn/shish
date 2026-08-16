#include "../fd.h"
#include "../parse.h"
#include "../sh.h"
#include "../../lib/str.h"
#include "../vartab.h"
#include <assert.h>

/* add a new variable using the supplied var struct rather
 * than a malloced
 * ----------------------------------------------------------------------- */
struct var*
var_import(const char* v, int flags, struct var* var) {
  struct search ctx;
  struct var* newvar;

  /* we should be in the root to import vars */
  assert(sh->varstack == &vartab_root);

  /* variable name must be valid! */
  if(!var_valid(v))
    return var;

  /* search if the var already exists */
  vartab_hash(v, &ctx);

  if(!(newvar = var_search(v, &ctx))) {
    /* if not we take the supplied var struct,  */
    newvar = var;
    var = NULL;
    var_init(v, newvar, &ctx);

    /* var_init() allocates its own copy of the name into sa; this
       path instead points sa.s at the caller's borrowed storage (v,
       typically straight into envp[]), so free that allocation and
       reset sa.a to 0 to avoid later freeing memory we don't own. */
    stralloc_free(&newvar->sa);
    newvar->sa.a = 0;

    /* ...and then add it to the table */
    vartab_add(sh->varstack, newvar, &ctx);
  } else if(flags & V_INIT)
    return var;

  newvar->flags = flags;
  newvar->sa.s = (char*)v;
  newvar->sa.len = str_len(v);

  if(newvar->sa.len > newvar->len)
    newvar->offset = newvar->len + 1;

  return newvar;
}
