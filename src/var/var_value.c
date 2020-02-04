#include "../var.h"

/* get a variable value
 * ----------------------------------------------------------------------- */
const char*
var_value(const char* v, size_t* plen) {
  return var_vdefault(v, "", plen);
}
