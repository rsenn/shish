#include "../vartab.h"
#include <stdlib.h>

/* get a variable
 * ----------------------------------------------------------------------- */
const char*
var_get(const char* v, size_t* offset) {
  struct var* var;

  if((var = var_search(v, NULL))) {
    if(offset)
      *offset = var->offset;

    if(var->flags & V_CALL) {
      ssize_t r;

      while((r = var->call(var->sa.s, var->sa.len)) < 0)
        stralloc_readyplus(&var->sa, 16);

      var->sa.len = r;
    }

    return var->sa.s;
  }

  return 0;
}
