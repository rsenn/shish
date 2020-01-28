#include "parse.h"
#include "source.h"
#include "tree.h"

/* parse arithmetic unary expression
 * ----------------------------------------------------------------------- */
union node*
parse_arith_unary(struct parser* p) {
  union node* node;
  enum nod_id n;

  char c, c2 = 0;
  if(source_peek(&c) <= 0)
    return 0;

  if((c == '+' || c == '-')) {
    if(source_peekn(&c2, 1) <= 0)
      return 0;
  }

  switch(c) {
    case '!': n = A_NOT; break;
    case '~': n = A_BNOT; break;
    case '-': n = c2 == '-' ? A_PREDECREMENT : A_UNARYMINUS; break;
    case '+': n = c2 == '+' ? A_PREINCREMENT : A_UNARYPLUS; break;
    default: return parse_arith_value(p);
  }

  if(c == c2)
    source_skip();

  source_skip();
  parse_skipspace(p);

  node = tree_newnode(n);
  node->narithunary.node = parse_arith_value(p);

  return node;
}
