#include "tree.h"
#include "parse.h"
#include "source.h"

/* parse arithmetic unary expression
 * ----------------------------------------------------------------------- */
union node*
parse_arith_unary(struct parser *p) {
  char c;
  if(source_peek(&c) <= 0)
    return 0;

  if(!(c == '!' || c == '~'))
    return NULL;

  source_skip();

  union node *node = tree_newnode(c == '!' ? N_ARITH_NOT : N_ARITH_BNOT);

  node->narithunary.node = parse_arith_value(p);

  return node;
}

