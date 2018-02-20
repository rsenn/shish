#include "tree.h"
#include "parse.h"
#include "source.h"

/* parse arithmetic binary expression
 * ----------------------------------------------------------------------- */
union node*
parse_arith_additive(struct parser *p) {
  char c;
  enum nod_id id = -1;
  union node* n = parse_arith_unary(p);

  parse_skipspace(p);

  if(source_peek(&c) <= 0)
    return 0;

  switch(c) {
    case '*': id = N_ARITH_MUL; break;
    case '/': id = N_ARITH_DIV; break;
    case '%': id = N_ARITH_MOD; break;
    default: return n;
  }

  union node* newnode = tree_newnode(id);
  newnode->narithbinary.left = n;
  newnode->narithbinary.right = parse_arith_unary(p);

  return newnode;
}

union node*
parse_arith_multiplicative(struct parser *p) {
  return 0;
}

union node*
parse_arith_shift(struct parser *p) {
  return 0;
}


union node*
parse_arith_relational(struct parser *p) {
  return 0;
}


union node*
parse_arith_equality(struct parser *p) {
  return 0;
}


union node*
parse_arith_and(struct parser *p) {
  return 0;
}


union node*
parse_arith_xor(struct parser *p) {
  return 0;
}


union node*
parse_arith_or(struct parser *p) {
  return 0;
}

union node*
parse_arith_logical_and(struct parser *p) {
  return 0;
}

union node*
parse_arith_logical_or(struct parser *p) {
  return 0;
}

