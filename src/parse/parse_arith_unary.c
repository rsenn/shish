#include "tree.h"
#include "parse.h"
#include "source.h"

union node*
parse_arith_postix(struct parser *p) {
  char c[2];
  enum nod_id id = -1;
  union node *postfix, *n = parse_arith_primary(p);

  parse_skipspace(p);

  if(source_peekn(c, 2) <= 1)
    return 0;

  switch(c[0] | (c[1] << 8)) {
  case 0x2b2b: id = N_ARITH_POSTINC; break;
  case 0x2d2d: id = N_ARITH_POSTDEC; break;
  default: return n;
  }

  parse_skipspace(p);
  parse_skipspace(p);

  postfix = tree_newnode(id);
  postfix->narithunary.node = n;

  return postfix;
}

/* parse arithmetic unary expression
 * ----------------------------------------------------------------------- */
union node*
parse_arith_unary(struct parser *p) {
  char c;

  parse_skipspace(p);

  if(source_peek(&c) <= 0)
    return 0;

  if(!(c == '!' || c == '~' || c == '+' || c == '-'))
    return parse_arith_primary(p);

  source_skip();

  union node *node = tree_newnode(c == '!' ? N_ARITH_NOT : N_ARITH_BNOT);

  node->narithunary.node = parse_arith_value(p);

  return node;
}

