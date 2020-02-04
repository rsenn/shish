#include "../expand.h"
#include "../parse.h"
#include "../tree.h"

/* parse a string part of a word and add it to the tree in parse_node
 * ----------------------------------------------------------------------- */
void
parse_string(struct parser* p, int flags) {
  if(p->sa.len == 0 && !p->quot)
    return;

  /* add a node if there is none */
  if(p->tree == NULL) {
    parse_newnode(p, N_ARGSTR);
    p->node->nargstr.flag = p->quot | flags;
  }

  /* add a node if required */
  if(p->node->id != N_ARGSTR || (p->node->nargstr.flag & S_TABLE) != p->quot)
    parse_newnode(p, N_ARGSTR);

  /* add string data to the node */
  p->node->nargstr.flag |= p->quot | flags;

  if(p->sa.len)
    stralloc_catb(&p->node->nargstr.stra, p->sa.s, p->sa.len);

  stralloc_nul(&p->node->nargstr.stra);
  stralloc_zero(&p->sa);
  stralloc_nul(&p->sa);
}
