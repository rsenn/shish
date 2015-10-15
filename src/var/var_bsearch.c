#include <stdlib.h>
#include "vartab.h"

/* find the next variable that is bigger or equal to the wanted one and
 * then do lexicographical weighing on them.
 * 
 * when it finds a match it will return 0 and context->pos will be set
 * to a pointer to the matching vartab entry
 * 
 * when the last checked variable was bigger than the wanted one
 * we return the hash distance from the wanted to bigger entry.
 * ----------------------------------------------------------------------- */
unsigned long var_bsearch(struct search *context) {
  struct var *var;
  
  /* continue looping through the current list */
  for(; (var = *context->pos); 
      context->pos = (context->global ? &var->gnext : &var->bnext)) {
    const char *w = context->name;
    const char *m = var->sa.s;
    unsigned long lw = context->len;
    unsigned long lm = var->len;
    long ret = 0;

    /* compare char by char */
    while(lw && lm && (ret = (*m - *w)) == 0) {
      w++; lw--;
      m++; lm--;
    }
    
    if(ret < 0) continue;
    if(ret > 0) return ret;
    
    if(lm == 0) {
      if(lw == 0) return 0;
      return (unsigned long)*w;
    }
  }
  
  return -1;
}

