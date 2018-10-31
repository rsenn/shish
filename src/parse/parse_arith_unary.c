#include "tree.h"
#include "parse.h"
#include "source.h"

/* parse arithmetic unary expression
 * ----------------------------------------------------------------------- */
union node*
parse_arith_unary(struct parser *p) {
  union node *node;
  enum nod_id n;

  char c;
  if(source_peek(&c) <= 0)
    return 0;


  switch(c) {
    case '!': n = N_ARITH_NOT; break;
    case '~': n = N_ARITH_BNOT; break;
    case '-': n = N_ARITH_UNARYMINUS; break;
    case '+': n = N_ARITH_UNARYPLUS; break;
    default: return NULL;
  }

  source_skip();
  parse_skipspace(p);

  node = tree_newnode(n);
  node->narithunary.node = parse_arith_value(p);

  return node;
}

