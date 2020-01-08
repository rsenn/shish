#include "parse.h"
#include "source.h"
#include "tree.h"

/* parse arithmetic unary expression
 * ----------------------------------------------------------------------- */
union node*
parse_arith_unary(struct parser* p) {
  union node* node;
  enum nod_id n;

  char c;
  if(source_peek(&c) <= 0)
    return 0;

  switch(c) {
    case '!': n = A_NOT; break;
    case '~': n = A_BNOT; break;
    case '-': n = A_UNARYMINUS; break;
    case '+': n = A_UNARYPLUS; break;
    default: return parse_arith_value(p);
  }

  source_skip();
  parse_skipspace(p);

  node = tree_newnode(n);
  node->narithunary.node = parse_arith_value(p);

  return node;
}
