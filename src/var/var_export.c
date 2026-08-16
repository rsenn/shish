#include "../../lib/byte.h"
#include "../var.h"
#include <stdlib.h>

/* export variables for execve()
 * ----------------------------------------------------------------------- */
char**
var_export(char** dest) {
  struct var* var;
  size_t n = 0;

  for(var = var_list; var; var = var->gnext)
    /* a variable exported before ever being assigned (V_UNSET) has no
       "=value" -- skip it, matching bash's "export FOO" not appearing
       in a child's environment until FOO is assigned. */
    if((var->flags & V_EXPORT) && !(var->flags & V_UNSET))
      dest[n++] = var->sa.s;

  dest[n] = NULL;

  return dest;
}
