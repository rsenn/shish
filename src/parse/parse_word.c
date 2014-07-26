#include <scan.h>
#include "tree.h"
#include "parse.h"
#include "source.h"

/* parse a word token
 * ----------------------------------------------------------------------- */
int parse_word(struct parser *p)
{
  unsigned char c;

  /* if there is still a tree from the last call then remove it */
  if(p->tree)
    tree_free(p->tree);
  
  p->tree = NULL;
  p->node = NULL;
  p->quot = Q_UNQUOTED;
  
  /* initialize string data */
  stralloc_zero(&p->sa);

  p->tok = T_NAME;
  
  for(;;)
  {
    if(source_peek(&c) <= 0)
      break;
    
    switch(p->quot)
    {
      case Q_DQUOTED: if(!parse_dquoted(p)) continue; break;
      case Q_SQUOTED: if(!parse_squoted(p)) continue; break;
      default: if(!parse_unquoted(p)) continue; break;
    }
    
    break;
  }
  
  parse_string(p, 0);    
  return p->tok;
}


