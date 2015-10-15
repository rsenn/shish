#include "shell.h"
#include <assert.h>
#include "vartab.h"

/* clean all vars on a variable table
 * ----------------------------------------------------------------------- */
void vartab_cleanup(struct vartab *vartab) {
  unsigned int i;
  
  /* loop through topmost vartab */
  for(i = 0; i < VARTAB_BUCKETS; i++) {
    struct var *var;
    struct var *next;
    
    for(var = vartab->table[i]; var; var = next) {
      next = var->bnext;

      assert(var->child == NULL);
      
      if(var->parent) {
        var->parent->child = NULL;
        
        if((var->parent->gnext = var->gnext))
          var->gnext->glink = &var->parent->gnext;
        
        var->parent->glink = var->glink;
       *var->glink = var->parent;
      } else {
        if((*var->glink = var->gnext))
          var->gnext->glink = var->glink;
      }
      
      if(var->flags & V_FREESTR)
        stralloc_free(&var->sa);
      
      /* if it was malloc()ed we have to free it now */
      if((var->flags & V_FREE))
        shell_free(var);
    }
    
    /* clear the hash bucket */
    vartab->table[i] = NULL;
  }
}

