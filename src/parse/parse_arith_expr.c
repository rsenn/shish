#include "../parse.h"
#include "../source.h"
#include "../tree.h"

/* parse arithmetic expression
 * ----------------------------------------------------------------------- */
union node*
parse_arith_expr(struct parser* p) {
  union node* node = 0;

  parse_skipspace(p);

  /* if(!node)
     node = parse_arith_binary(p, 9);
   if(node->id == N_ARGPARAM)*/
  node = parse_arith_assign(p, node);
  if(!node)
    node = parse_arith_unary(p);
  if(!node)
    node = parse_arith_value(p);

  return node;
}
