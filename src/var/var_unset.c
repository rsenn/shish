#include "../../lib/shell.h"
#include "../../lib/str.h"
#include "../var.h"

/* unset a variable
 * ----------------------------------------------------------------------- */
int
var_unset(char* v) {
  struct var* var;

  /* RANDOM's magic is permanently disabled by unset, even if it's never
     reassigned afterward -- matches bash, see var_random.c */
  if(str_equal(v, "RANDOM"))
    var_random_unset();

  /* find the variable */
  if((var = var_search(v, NULL)) == NULL)
    return 0;

  do {
    /*    if(var->child &&
         (var->child->flags & V_FREE))
          alloc_free(var->child);*/

    var_cleanup(var);
  } while((var = var->parent));
  return 1;
}
