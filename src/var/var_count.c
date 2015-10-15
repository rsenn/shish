#include "sh.h"
#include "var.h"

/* count variables having the specified flag set */
unsigned long var_count(int flags) {
  struct var *var;
  unsigned long n = 0;

  for(var = var_list; var; var = var->gnext)
    if((var->flags & flags) == flags)
      n++;
  
  return n;
}


