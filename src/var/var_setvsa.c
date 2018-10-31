#include "byte.h"
#include "var.h"

/* set a variable value from a stralloc in the format: word
 * 
 * if you specify the V_FREE flag the var table will take control over
 * the whole stralloc, leaving you with a freshly initialized one.
 * otherwise the contents of the stralloc are copied.
 * 
 * IMPORTANT: the name must be a valid posix shell variable name, or it will 
 *            fuck up the whole var table!
 * ----------------------------------------------------------------------- */
const char *var_setvsa(const char *name, stralloc *sa, int flags) {
  struct var *var;
  
  /* find/create new variable on top vartab */
  var = var_create(name, flags);

  /* variable has a stralloc of which we have control */
  if(var->flags & V_FREESTR) {
    var->sa.len = var->offset;
    stralloc_catc(&var->sa, '=');
    stralloc_cat(&var->sa, sa);
    stralloc_trunc(&var->sa, var->sa.len);  
    
    if(flags & V_FREESTR)
      stralloc_free(sa);
    if(flags & (V_FREESTR|V_ZEROSA))
      stralloc_zero(sa);
  }
  /* variable currently has no controlable stralloc.. . */
  else {
    /* look if we can take the one from the value */
    if(flags & (V_FREESTR | V_ZEROSA)) {
      var->sa = *sa;
      stralloc_trunc(&var->sa, var->offset + sa->len);
      /* move the value past the = character */
      byte_copyr(var->sa.s + var->offset, sa->len, var->sa.s);
      stralloc_init(sa);
    } else {
      stralloc_init(&var->sa);
      stralloc_trunc(&var->sa, var->offset + sa->len);
      byte_copy(var->sa.s + var->offset, sa->len, sa->s);
    }
    
    byte_copy(var->sa.s, var->len, name);
  }
  
  /* set flags */
  var->flags |= flags;
  return var->sa.s;
}

