#include "tree.h"
#include "parse.h"

#ifdef DEBUG
void parse_newnodedebug(const char *file, unsigned int line, struct parser *p, enum nod_id nod)
#else 
void parse_newnode(struct parser *p, enum nod_id nod)
#endif /* DEBUG */
{
  if(p->tree)
  {
#ifdef DEBUG
    p->node->list.next = tree_newnodedebug(file, line, nod);
#else
    p->node->list.next = tree_newnode(nod);
#endif /* DEBUG */
    p->node = p->node->list.next;
  }
  else
  {
#ifdef DEBUG
    p->node = tree_newnodedebug(file, line, nod);
#else
    p->node = tree_newnode(nod);
#endif /* DEBUG */
    p->tree = p->node;
  }
}
