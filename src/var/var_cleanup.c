#include "var.h"

void var_cleanup(struct var *var) {
  /* free if it was a previously allocated string */
  if(var->sa.a)
    stralloc_free(&var->sa);
  
  /* unlink from the bucket list */
  if((*var->blink = var->bnext))
    var->bnext->blink = var->blink;
  
  /* if we're in the global list unlink also from there */
  if(var->child == NULL) {
    if((*var->glink = var->gnext))
      var->gnext->glink = var->glink;
  }
}
