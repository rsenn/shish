#include "fmt.h"
#include "fd.h"
#include "var.h"

/* dump a variable entry
 * ----------------------------------------------------------------------- */
void var_dump(struct var *var) {
  char numbuf[FMT_XLONG*2];
  unsigned long n;

  /* var struct address */
  n = fmt_xlong(numbuf, (unsigned long)var);
  buffer_putnspace(fd_out->w, 8 - n);
  buffer_put(fd_out->w, numbuf, n);
  buffer_putspace(fd_out->w);
  
  /* variable name */
  if(var->len > 24) {
    buffer_put(fd_out->w, var->sa.s, 21);
    buffer_puts(fd_out->w, "...");
  } else {
    buffer_put(fd_out->w, var->sa.s, var->len);
    buffer_putnspace(fd_out->w, 24 - var->len);
  }
  
  /* variable value */
  n = var_vlen(var->sa.s);
  
  if(n) {
    unsigned int i, l, rl, rn;
    rn = var->sa.len - var->offset;
    rl = (rn > 24 ? 21 : rn);
    l = (n > 24 ? 21 : n);
    
    for(i = 0; i < rl; i++) {
      if(var->sa.s[var->offset + i] != '\n' && 
         var->sa.s[var->offset + i] != '\t')
        buffer_put(fd_out->w, &var->sa.s[var->offset + i], 1);
      else
        buffer_putspace(fd_out->w);
    }
    
    if(rl < rn) {
      buffer_puts(fd_out->w, "...");
      rl += 3;
      l += 3;
    }
    
    n = 24 - rl;
  } else
    n = 24;
  
  buffer_putnspace(fd_out->w, n);
  
  /* name length */
  n = fmt_ulong(numbuf, var->len);
  buffer_putnspace(fd_out->w, 5 - n);
  buffer_put(fd_out->w, numbuf, n);
  
  /* value offset */
  n = fmt_ulong(numbuf, var->offset);
  buffer_putnspace(fd_out->w, 5 - n);
  buffer_put(fd_out->w, numbuf, n);
  
  /* total length */
  n = fmt_ulong(numbuf, var->sa.len);
  buffer_putnspace(fd_out->w, 5 - n);
  buffer_put(fd_out->w, numbuf, n);
  
  /* vartab level */
  n = fmt_ulong(numbuf, var->table->level);
  buffer_putnspace(fd_out->w, 4 - n);
  buffer_put(fd_out->w, numbuf, n);
  
  /* vartab bucket */
  n = fmt_ulong(numbuf, (var->rndhash % VARTAB_BUCKETS));
  buffer_putnspace(fd_out->w, 5 - n);
  buffer_put(fd_out->w, numbuf, n);
  
  buffer_putspace(fd_out->w);
  
  /* lexical hash */
  n = fmt_xlonglong(numbuf, var->lexhash);
  buffer_putnspace(fd_out->w, 16 - n);
  buffer_put(fd_out->w, numbuf, n);
  buffer_putspace(fd_out->w);
  
  /* randomized hash */
  n = fmt_xlonglong(numbuf, var->rndhash);
  buffer_putnspace(fd_out->w, 16 - n);
  buffer_put(fd_out->w, numbuf, n);
  buffer_putnlflush(fd_out->w);
}

