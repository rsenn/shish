#include "sh.h"
#include "fd.h"
#include "vartab.h"

/* find a variable on any table
 * ----------------------------------------------------------------------- */
struct var *var_search(const char *v, struct search *context) {
  struct vartab *vartab;
  struct var *var = NULL;
  
  /* do a context inside here if none was supplied */
  if(context == NULL) {
    struct search ctx;
    vartab_hash(sh->varstack, v, &ctx);
    ctx.exact = 1;
    return var_search(v, &ctx);
  }

/*  buffer_puts(&fd_out->b, "searching var: ");
  buffer_puts(&fd_out->b, v);
  buffer_putnlflush(&fd_out->b);*/
  
  /* loop through tables */
  for(vartab = sh->varstack; vartab; vartab = vartab->parent) {
    /* exact match always returns immediately */
    if((var = vartab_search(vartab, v, context)))
      break;
  }
  
  /* next time the context is used we have to
     skip through the global sorted list to
     find a place to insert */
  context->global = 1;
  return var;
}

