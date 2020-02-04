#include "../sh.h"
#include "../var.h"

/* count variables having the specified flag set */
size_t
var_count(int flags) {
  struct var* var;
  size_t n = 0;

  for(var = var_list; var; var = var->gnext)
    if((var->flags & flags) == flags)
      n++;

  return n;
}
