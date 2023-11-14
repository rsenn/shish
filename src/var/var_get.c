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
    return var->sa.s;
  }

  return 0;
}
