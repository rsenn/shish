#include "../parse.h"
#include "../tree.h"
#include "../expand.h"
#include "../../lib/str.h"
#include "../../lib/stralloc.h"

/* 3.9.5 - parse function definition
 * ----------------------------------------------------------------------- */
union node*
parse_function(struct parser* pp) {
  enum tok_id tok;
  union node* node;
  struct parser p;

  stralloc name;
  stralloc_init(&name);
  tok = parse_gettok(pp, 0);
  expand_tosa(pp->tree, &name);
  node = tree_newnode(N_FUNCTION);
  stralloc_nul(&name);
  node->nfunc.name = name.s;

  parse_init(&p, P_SKIPNL | pp->flags);
  node->nfunc.body = parse_grouping(&p);
  return node;
}
