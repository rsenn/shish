#include <stdlib.h>
#include "tree.h"
#include "redir.h"

struct nredir *redir_list = NULL;

/* add a here-doc
 * ----------------------------------------------------------------------- */
void redir_addhere(struct nredir *nredir)
{
  static struct nredir **rptr;
  
  if(redir_list == NULL)
    rptr = &redir_list;
        
  *rptr = nredir;
  rptr = (struct nredir **)&nredir->data;
}   

