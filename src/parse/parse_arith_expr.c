#include "../parse.h"
#include "../source.h"
#include "../tree.h"

/* parse arithmetic expression
 * ----------------------------------------------------------------------- */
union node*
parse_arith_expr(struct parser* p) {
  union node* node;
  /* char c = 0, c2 = 0; */

  parse_skipspace(p);

  node = parse_arith_assign(p);
  if(!node)
    node = parse_arith_binary(p, 9);
  if(!node)
    node = parse_arith_unary(p);
  if(!node)
    node = parse_arith_value(p);

  return node;
}
