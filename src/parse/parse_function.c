#include "../parse.h"
#include "../tree.h"
#include "../expand.h"
#include "../fd.h"
#include "../../lib/str.h"
#include "../../lib/stralloc.h"

/* 3.9.5 - parse function definition
 * ----------------------------------------------------------------------- */
union node*
parse_function(struct parser* p) {
  int tok;
  union node* node;
  char c;
  size_t i;
  char b[64];

  stralloc name;
  stralloc_init(&name);
  tok = parse_gettok(p, 0);
  expand_tosa(p->tree, &name);
  node = tree_newnode(N_FUNCTION);
  stralloc_nul(&name);
  node->nfunc.name = name.s;

  do
    tok = parse_gettok(p, 0);
  while(tok == T_NL || tok == T_NAME);
  p->pushback++;
  /*
    if((tok = parse_gettok(p, 0)) != T_BEGIN) {
      tree_free(node);
      return NULL;
    }*/
  if(!parse_expect(p, P_SKIPNL, T_BEGIN | T_LP, node))
    return NULL;

  p->pushback++;
  node->nfunc.body = parse_grouping(p, P_SKIPNL);
  return node;
}