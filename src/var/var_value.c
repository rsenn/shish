#include "var.h"

/* get a variable value
 * ----------------------------------------------------------------------- */
const char *var_value(const char *v, unsigned long *plen) {
  return var_vdefault(v, "", plen);
}


