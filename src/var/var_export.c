#include "../../lib/byte.h"
#include "var.h"
#include <stdlib.h>

/* export variables for execve()
 * ----------------------------------------------------------------------- */
char**
var_export(char** dest) {
  struct var* var;
  size_t n = 0;

  for(var = var_list; var; var = var->gnext)
    if((var->flags & V_EXPORT))
      dest[n++] = var->sa.s;

  dest[n] = NULL;

  return dest;
}
