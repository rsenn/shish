#include "../../lib/alloc.h"
#include "../sh.h"
#include "../../lib/shell.h"
#include "../vartab.h"

/* create a new var on top vartab, possibly overwriting an old one
 *
 * when a variable is found on the top table it is immediately returned,
 * if found on a
 * ----------------------------------------------------------------------- */
struct var*
var_create(const char* s, int flags) {
  struct search ctx;
  struct var *newv, *v;
  struct vartab* tab = varstack;

  vartab_hash(s, &ctx);

  if((v = var_search(s, &ctx))) {
    /* if we have the V_INIT flag and the var was found return NULL */
    if(flags & V_INIT)
      return NULL;

    /* if variable was found on topmost level -> immediately return it */
    if(v->table == tab)
      return v;

    if(v->flags & V_LOCAL)
      return v;
  }

  if(!(flags & V_LOCAL)) {
    while(tab->function)
      tab = tab->parent;

    /* if variable is found on that table -> immediately return it */
    if(v && v->table == tab)
      return v;
  }

  newv = alloc(sizeof(struct var));
  var_init(s, newv, &ctx);
  newv->flags |= V_FREE;

  /* if the variable was found on another
     level then do some pointer setup :) */
  if(v) {
    v->child = newv;
    newv->parent = v;

    newv->sa = v->sa;
    newv->sa.a = 0;
  }

  /* finally add it to the bucket and to the global list */
  vartab_add(tab, newv, &ctx);
  return newv;
}
