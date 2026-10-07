#include "../../lib/alloc.h"
#include "../../lib/shell.h"
#include "../../lib/str.h"
#include "../var.h"
#include "../trace.h"

/* unset a variable
 * ----------------------------------------------------------------------- */
int
var_unset(char* v) {
  struct var *var, *parent;

  TRACE(TRACE_VAR, "unset", trace_str("name", v), trace_int("level", varstack->level));

  /* RANDOM's magic is permanently disabled by unset, even if it's never
     reassigned afterward -- matches bash, see var_random.c */
  if(str_equal(v, "RANDOM"))
    var_random_unset();

  /* find the variable */
  if((var = var_search(v, NULL)) == NULL)
    return 0;

  /* every shadowed level goes too; the node itself is ours when var_create() made it */
  do {
    parent = var->parent;
    var_cleanup(var);

    if(var->flags & V_FREE)
      alloc_free(var);
  } while((var = parent));
  return 1;
}
