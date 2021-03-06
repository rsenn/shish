#include "../sh.h"
#include "../../lib/shell.h"
#include "../vartab.h"

/* create a new var on top vartab, possibly overwriting an old one
 *
 * when a variable is found on the top table it is immediately returned,
 * if found on a
 * ----------------------------------------------------------------------- */
struct var*
var_create(const char* v, int flags) {
  struct search ctx;
  struct var *newvar, *oldvar;

  vartab_hash(v, &ctx);
  if((oldvar = var_search(v, &ctx))) {
    /* if we have the V_INIT flag and the var was found return NULL */
    if(flags & V_INIT)
      return NULL;

    /* if variable was found on topmost level -> immediately return it */
    if(oldvar->table == sh->varstack)
      return oldvar;
  }

  newvar = shell_alloc(sizeof(struct var));
  newvar->flags |= V_FREE;
  var_init(v, newvar, &ctx);

  /* if the variable was found on another
     level then do some pointer setup :) */
  if(oldvar) {
    oldvar->child = newvar;
    newvar->parent = oldvar;

    newvar->sa = oldvar->sa;
    newvar->sa.a = 0;
  }

  {
    struct vartab* tab = sh->varstack;

    if(!(flags & V_LOCAL) && tab->function)
      tab = tab->parent;

    /* finally add it to the bucket and to the global list */
    vartab_add(tab, newvar, &ctx);
  }
  return newvar;
}
