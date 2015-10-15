#include "var.h"
#include <assert.h>

/* set a variable from a stralloc in the format: name=word
 * 
 * if you specify the V_FREE flag the var table will take control over
 * the whole stralloc, leaving you with a freshly initialized one.
 * otherwise the contents of the stralloc are copied.
 * 
 * IMPORTANT: the name must be a valid posix shell variable name, or it will 
 *            fuck up the whole var table!
 * ----------------------------------------------------------------------- */
struct var *var_setsa(stralloc *sa, int flags) {
  struct var *var;
  
  /* goddamn, haven't i told you? */
  assert(sa->len);
  
  /* make sure the variable is nul-terminated, 
     in case its not terminated with an equal sign */
  stralloc_nul(sa);
  
  /* find/create new variable on top vartab */
  var = var_create(sa->s, flags);

  /* now check how we set the value, there are 4 possibilities */  
  if(var->flags & V_FREESTR) {
    /* if both, old and new variable should be freed
       then free the old one */
    if(flags & V_FREESTR)
      stralloc_free(&var->sa);
    
    /* the old one should be freed, but the new one not,
       so we can copy new to old */
    stralloc_zero(&var->sa);
  } else {
    stralloc_init(&var->sa);
  }
  
  /* new arg should be freed when the var gets removed,
     so we can just move it */
  if(flags & V_FREESTR) {
    var->sa = *sa;
    stralloc_init(sa);
  }
  /* the source should not be free'd, so copy */
  else {
    /* src should not be freed */
    stralloc_copy(&var->sa, sa);
  }
 
  var->flags |= flags;
  
  /* truncate and set flags */
  stralloc_nul(&var->sa);
  
  if(var->sa.len > var->len)
    var->offset = var->len + 1;
  
  return var;
}

