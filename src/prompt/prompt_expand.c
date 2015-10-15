#include <stdlib.h>
#include "tree.h"
#include "debug.h"
#include "prompt.h"
#include "expand.h"

stralloc    prompt_expansion = { NULL, 0, 0 };     /* expanded PS1 */
union node *prompt_node = NULL;

/* expands the prompt if necessary
 * ----------------------------------------------------------------------- */
void prompt_expand(void) {
  /* expand PS1 only */
  if(prompt_number != 1)
    return;

  /* expand prompt tree if present */
  if(prompt_node) {
    stralloc sa;

    /* escape prompt */
    stralloc_init(&sa);
#ifdef DEBUG
//    debug_list(prompt_node, 0);      
#endif    
    expand_catsa(prompt_node, &sa, 0);
    stralloc_nul(&sa);
    stralloc_zero(&prompt_expansion);
    prompt_escape(sa.s, &prompt_expansion);
    stralloc_nul(&prompt_expansion);

#ifdef DEBUG
//    debug_stralloc("prompt", &sa, 0);
#endif      
    
  }
}

