#include <unistd.h>
#include "byte.h"
#include "shell.h"
#include "sh.h"

/* after forking, returns pid
 * ----------------------------------------------------------------------- */
int sh_forked(void) {
  struct env *e = sh;
  struct env *next;
  
  /* if we're not in the root environment we clean up any shell env */
  for(sh = sh->parent; sh; sh = next) {
    next = sh->parent;
      
    sh_setargs(NULL, 0);
    
    if(sh->cwd.a)
      stralloc_free(&sh->cwd);
    else
      sh->cwd.s = NULL;
  }
  
  sh = &sh_root;
  byte_copy(sh, sizeof(struct env), e);
  sh->parent = NULL;
  sh->jump = 0;
  sh_child = 1;
  
  return 0;//(sh_pid = getpid());
}


