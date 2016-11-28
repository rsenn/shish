#include "fd.h"
#include "vartab.h"
#include "var.h"
#include "debug.h"
#include <assert.h>

void vartab_add(struct vartab *vartab, struct var *var, struct search *context) {
  var->table = vartab;
  
  /* its already in this table? wtf? */
  assert(var != *vartab->pos);
    
/*  debug_str("linking", context->name, 0);*/
  
  /* link it to the list */
  var->blink = vartab->pos;
  
  if((var->bnext = *var->blink))
    var->bnext->blink = &var->bnext;
  
 *var->blink = var;
  
  /* if the variable has a parent we already know the position in the global list :D */
  if(var->parent) {
    assert(var->parent->table != var->table);
  
    /* link ourselves instead of our parent into the global list */
    if((var->gnext = var->parent->gnext))
      var->gnext->glink = &var->gnext;
    
    var->glink = var->parent->glink;
   *var->glink = var;
  } else {
    /* start searching insert position in global list */
/*    if(context->closest && context->closest != &vartab->table[context->bucket])
      context->pos = context->closest;
    else*/
      context->pos = &var_list;
    
    context->global = 1;
    
    /*  context->pos = &varlist;*/
    /* */
    /* then we search forward again until the next entry is bigger */
    if(var_hsearch(context) == 0LL) {
#ifdef DEBUG
      unsigned long dist =
#endif
        var_bsearch(context);
      
      /* there must not be a full match, otherwise this means that
         the variable is already in the list which it should not! */
#ifdef DEBUG
      assert(dist != 0L); 
#endif
    }
    
    var->glink = context->pos;

    if((var->gnext = *var->glink))
      var->gnext->glink = &var->gnext;
    
   *var->glink = var;
  }
}
