#include <assert.h>
#include "str.h"
#include "parse.h"
#include "fd.h"
#include "var.h"

/* print a variable, suitable for re-input
 * ----------------------------------------------------------------------- */
void var_print(struct var *var, int flags) {
  if(flags & V_EXPORT)
    buffer_puts(fd_out->w, "export ");

  /* output variable name */
  buffer_put(fd_out->w, var->sa.s, var->len);

  /* if the variable wasn't unset we display it */
  if(!(var->flags & V_UNSET)) {
    unsigned long i;
    
    buffer_puts(fd_out->w, "=\"");

    for(i = var->offset; i < var->sa.len; i++) {
      /* escape characters that must be escaped in double-quotation mode */
      if(parse_isdesc(var->sa.s[i]))
        buffer_puts(fd_out->w, "\\");

      buffer_PUTC(fd_out->w, var->sa.s[i]);
    }
    
    buffer_puts(fd_out->w, "\"");
  }
  
  buffer_putnlflush(fd_out->w);
}

