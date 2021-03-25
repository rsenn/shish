#include "../expand.h"
#include "../parse.h"
#include "../tree.h"
#include <assert.h>

/* parse a string part of a word and add it to the tree in parse_node
 * ----------------------------------------------------------------------- */
void
parse_string(struct parser* p, int flags) {
  struct nargstr* arg;
  if(p->sa.len == 0 && !p->quot)
    return;

  /* add a node if there is none */
  if(p->node == NULL || p->node->id != N_ARGSTR || (p->node->nargstr.flag & S_TABLE) != p->quot) {
    parse_newnode(p, N_ARGSTR);
    p->node->nargstr.flag = p->quot | flags;
    p->node->nargstr.pos = p->tokstart;
  }

  assert(p->node);
  assert(p->node->id == N_ARGSTR);

  arg = &p->node->nargstr;

  /* add string data to the node */
  arg->flag |= p->quot | flags;

  if(p->sa.len)
    stralloc_catb(&arg->stra, p->sa.s, p->sa.len);

  stralloc_nul(&arg->stra);
  stralloc_zero(&p->sa);
  stralloc_nul(&p->sa);
}
