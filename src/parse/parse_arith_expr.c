#include "tree.h"
#include "parse.h"
#include "source.h"

/* parse arithmetic expression
 * ----------------------------------------------------------------------- */


union node*
parse_arith_conditional(struct parser *p) {
  union node* n;
  char c;

  n = parse_arith_logical_or(p);

  parse_skipspace(p);
  source_peek(&c);

  if(c == '?') {
     union node* ternary = tree_newnode(N_ARITH_TERNARY);
     ternary->narithternary.cond = n;
     ternary->narithternary.true = parse_arith_expression(p);

     parse_skipspace(p);
     source_peek(&c);

     if(c != ':') {
       tree_free(ternary);
       return NULL;
     }
     source_skip();

     ternary->narithternary.false = parse_arith_conditional(p);
     n = ternary;
  }

  return n;
}

/* ----------------------------------------------------------------------- */
union node*
parse_arith_assignment(struct parser *p) {
  union node* n;

  n = parse_arith_conditional(p);

}

/* ----------------------------------------------------------------------- */
union node*
parse_arith_expression(struct parser *p) {
  union node *n;

  parse_skipspace(p);

/*  n = parse_arith_unary(p);
  if(!n)*/ n = parse_arith_additive(p);
  if(!n) n = parse_arith_value(p);

  return n;
}

/* ----------------------------------------------------------------------- */
union node*
parse_arith_constant_expression(struct parser *p) {
  union node *n = parse_arith_conditional(p);
  return n;
}
