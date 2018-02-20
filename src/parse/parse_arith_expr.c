#include "tree.h"
#include "parse.h"
#include "source.h"

/* parse arithmetic expression
 * ----------------------------------------------------------------------- */
union node*
parse_arith_expr(struct parser *p) {
  union node *n;

  parse_skipspace(p);

/*  n = parse_arith_unary(p);
  if(!n)*/ n = parse_arith_additive(p);
  if(!n) n = parse_arith_value(p);

  return n;
}

