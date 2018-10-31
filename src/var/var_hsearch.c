#include <stdlib.h>
#include "vartab.h"

/* find the next variable that is bigger or equal to the wanted one and
 * then do a hashed weighing on them.
 * 
 * when it finds a match it will return 0 and context->pos will be set
 * to a pointer to the matching vartab entry
 * 
 * when the last checked variable was bigger than the wanted one
 * we return the hash distance from the wanted to bigger entry.
 * ----------------------------------------------------------------------- */
VAR_HASH var_hsearch(struct search *context) {
  struct var *var;
 
  /* continue looping through the current list */
  for(; (var = *context->pos); 
      context->pos = (context->global ? &var->gnext : &var->bnext)) {
    /* because the list is sorted ascending we can skip all
       the vars while their literal hash is smaller than the
       one of the wanted var */
    if(var->lexhash < context->lexhash)
      continue;

    /* if we are heading for an exact match (i.e. not creating
       any variable after not found) we do  not require the exact
       distance and we can also continue on a rhash or len mismatch */
    if(context->exact) {
      /* in this case those two conditionals, especially the
         rndhash match will prevent from unneccessary lexical
         weighing */
      if(context->len != var->len)
        continue;
      
      /* a 100% mismatch, but without any possibility to calc the
         real distance :) */
      if(context->rndhash != var->rndhash) 
        continue;
    }
 
    /* at this point var->lexhash must be bigger or equal to context->lexhash,
       so a positive distance is returned */
    return var->lexhash - context->lexhash;
  }
  
  /* looks like there was no entry, or we were bigger than any var,
     return the maximum possible distance */
  return (VAR_HASH)-1LL;
}
