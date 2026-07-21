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

    /* var_init() now allocates its own copy of the name into sa (see
       its comment) so sa.s is never NULL -- but this path is about to
       point sa.s directly at the caller's own storage instead (v,
       typically a pointer straight into the process's real envp[]),
       so free that allocation and reset sa.a to 0 right away. Leaving
       it non-zero here would claim ownership of borrowed memory: the
       next stralloc_free() on this var (e.g. from var_set()/
       var_setsa() on a later real assignment) would try to
       alloc_free() a pointer that was never malloc()'d by us. */
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
