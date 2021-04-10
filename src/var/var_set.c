#include "../../lib/byte.h"
#include "../../lib/str.h"
#include "../var.h"
#include "../sh.h"
#include "../fd.h"

/* set a variable
 * ----------------------------------------------------------------------- */
struct var*
var_set(char* v, int flags) {
  struct var* var;

  /* find/create the variable */
  if((var = var_create(v, flags)) == NULL)
    return var;

  if(var->flags & V_READONLY) {
    sh_msgn(var->sa.s, var->len);
    buffer_putsflush(fd_err->w, ": readonly variable\n");
    return 0;
  }

  if(v == var->sa.s)
    return var;

  if(v[var->len] == '\0') {
    var->sa.len = var->offset;
    stralloc_nul(&var->sa);
    return var;
  }

  /* free if it was a previously allocated string */
  if(var->sa.a)
    stralloc_free(&var->sa);

  stralloc_init(&var->sa);

  var->sa.s = v;
  var->sa.len = str_len(v);

  if((var->offset = var->len) < var->sa.len)
    var->offset++;

  return var;
}
