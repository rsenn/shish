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
    /* a variable marked for export before it was ever assigned a
       value (V_UNSET, see var_init.c) has no "=value" to hand a
       child process -- its bare name alone (var->sa.s without a '=')
       would be a malformed envp entry, so skip it, matching bash's
       "export FOO" not actually appearing in a child's environment
       until FOO is assigned */
    if((var->flags & V_EXPORT) && !(var->flags & V_UNSET))
      dest[n++] = var->sa.s;

  dest[n] = NULL;

  return dest;
}
