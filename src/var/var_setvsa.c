#include "../../lib/byte.h"
#include "../var.h"

/* set a variable value from a stralloc in the format: word
 * - V_FREE: the var table takes control of the whole stralloc,
 *   leaving you with a freshly initialized one.
 * - otherwise: the stralloc's contents are copied.
 * name must be a valid POSIX shell variable name, or it corrupts the
 * whole var table.
 *
 *   const char*  name    variable name; must satisfy var_valid(name)
 *   stralloc*    sa      value to assign; see V_FREE above
 *   int          flags   V_FREE, V_ZEROSA, and the usual V_* var flags
 * ----------------------------------------------------------------------- */
const char*
var_setvsa(const char* name, stralloc* sa, int flags) {
  struct var* var;

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

    if(flags & (V_FREESTR | V_ZEROSA))
      stralloc_zero(sa);
  }
  /* variable currently has no controlable stralloc.. . */
  else {
    /* var_init() always allocates a (small, name-only) buffer for a
       freshly created var now, whether or not V_FREESTR ends up set
       -- free it before either branch below unconditionally
       overwrites/replaces var->sa, or it leaks */
    stralloc_free(&var->sa);
    var->sa.a = 0;

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
  var->flags &= ~V_UNSET;
  return var->sa.s;
}
