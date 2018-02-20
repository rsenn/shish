#include "tree.h"
#include "parse.h"
#include "source.h"

/* parse arithmetic additive expression
 * ----------------------------------------------------------------------- */
union node*
parse_arith_multiplicative(struct parser *p) {
  char c;
  enum nod_id id = -1;
  union node *n, *multiplicative;

  n = parse_arith_unary(p);

  parse_skipspace(p);

  if(source_peek(&c) <= 0)
    return 0;

  switch(c) {
    case '*': id = N_ARITH_MUL; break;
    case '/': id = N_ARITH_DIV; break;
    case '%': id = N_ARITH_MOD; break;
    default: return n;
  }

  source_skip();

  multiplicative = tree_newnode(id);
  multiplicative->narithbinary.left = n;
  multiplicative->narithbinary.right = parse_arith_multiplicative(p);

  return multiplicative;
}

union node*
parse_arith_additive(struct parser *p) {
  char c;
  enum nod_id id = -1;
  union node *n, *additive;

  n = parse_arith_multiplicative(p);

  parse_skipspace(p);

  if(source_peek(&c) <= 0)
    return 0;

  switch(c) {
  case '+': id = N_ARITH_ADD; break;
  case '-': id = N_ARITH_SUB; break;
  default: return n;
  }

  source_skip();

  additive = tree_newnode(id);
  additive->narithbinary.left = n;
  additive->narithbinary.right = parse_arith_additive(p);

  return additive;
}

union node*
parse_arith_shift(struct parser *p) {
  char c[2];
  enum nod_id id = -1;
  union node *n, *shift;

  n = parse_arith_additive(p);

  parse_skipspace(p);

  if(source_peekn(c, 2) <= 1)
    return 0;

  switch(c[0] | (c[1] << 8)) {
  case 0x3c3c: id = N_ARITH_LSHIFT; break;
  case 0x3e3e: id = N_ARITH_RSHIFT; break;
  default: return n;
  }

  source_skip();
  source_skip();

  shift = tree_newnode(id);
  shift->narithbinary.left = n;
  shift->narithbinary.right = parse_arith_shift(p);

  return shift;
}


union node*
parse_arith_relational(struct parser *p) {
  char c[2];
  enum nod_id id = -1;
  union node *n, *relational;

  n = parse_arith_shift(p);

  parse_skipspace(p);

  if(source_peek(&c[0]) <= 0)
    return 0;

  switch(c[0]) {
  case 0x3c:
  case 0x3e:
    if(source_peekn(c, 2) <= 1)
      return 0;

    id = (c[0] == '<' ? N_ARITH_LT : N_ARITH_GT);

    switch(c[0] | (c[1] << 8)) {
    case 0x3d3c:
    case 0x3d3e:
      id = (c[0] == '<' ? N_ARITH_LE : N_ARITH_GE);
      source_skip();
      break;
    }
    break;
  default: return n;
  }

  source_skip();

  relational = tree_newnode(id);
  relational->narithbinary.left = n;
  relational->narithbinary.right = parse_arith_relational(p);

  return relational;
}


union node*
parse_arith_equality(struct parser *p) {
  char c[2];
  enum nod_id id = -1;
  union node *n, *equality;

  n = parse_arith_relational(p);

  parse_skipspace(p);

  if(source_peekn(c, 2) <= 1)
    return 0;

  switch(c[0] | (c[1] << 8)) {
  case 0x213d: id = N_ARITH_NE; break;
  case 0x3d3d: id = N_ARITH_EQ; break;
  default: return n;
  }

  source_skip();
  source_skip();

  equality = tree_newnode(id);
  equality->narithbinary.left = n;
  equality->narithbinary.right = parse_arith_equality(p);

  return equality;
}


union node*
parse_arith_and(struct parser *p) {
  char c[2];
  enum nod_id id = -1;
  union node *n, *band;

  n = parse_arith_equality(p);

  parse_skipspace(p);

  if(source_peekn(c, 2) <= 1)
    return 0;

  if(c[0] == '&' && c[1] != '&')
    id = N_ARITH_BAND;
  else
    return n;

  source_skip();

  band = tree_newnode(id);
  band->narithbinary.left = n;
  band->narithbinary.right = parse_arith_and(p);

  return band;
}

union node*
parse_arith_xor(struct parser *p) {
  char c;
  enum nod_id id = -1;
  union node *n, *bxor;

  n = parse_arith_and(p);

  parse_skipspace(p);

  if(source_peek(&c) <= 0)
    return 0;

  if(c == '^')
    id = N_ARITH_BXOR;
  else
    return n;

  source_skip();

  bxor = tree_newnode(id);
  bxor->narithbinary.left = n;
  bxor->narithbinary.right = parse_arith_xor(p);

  return bxor;
}

union node*
parse_arith_or(struct parser *p) {
  char c;
  enum nod_id id = -1;
  union node *n, *bor;

  n = parse_arith_xor(p);

  parse_skipspace(p);

  if(source_peek(&c) <= 0)
    return 0;

  if(c == '|')
    id = N_ARITH_BOR;
  else
    return n;

  source_skip();

  bor = tree_newnode(id);
  bor->narithbinary.left = n;
  bor->narithbinary.right = parse_arith_or(p);

  return bor;
}

union node*
parse_arith_logical_and(struct parser *p) {
  char c[2];
  enum nod_id id = -1;
  union node *n, *land;

  n = parse_arith_or(p);

  parse_skipspace(p);

  if(source_peekn(c, 2) <= 1)
    return 0;

  if(c[0] == '&' && c[1] == '&')
    id = N_ARITH_AND;
  else
    return n;

  source_skip();
  source_skip();

  land = tree_newnode(id);
  land->narithbinary.left = n;
  land->narithbinary.right = parse_arith_logical_and(p);

  return land;
}

union node*
parse_arith_logical_or(struct parser *p) {
  char c[2];
  enum nod_id id = -1;
  union node *n, *lor;

  n = parse_arith_logical_and(p);

  parse_skipspace(p);

  if(source_peekn(c, 2) <= 1)
    return 0;

  if(c[0] == '|' && c[1] == '|')
    id = N_ARITH_OR;
  else
    return n;

  source_skip();
  source_skip();

  lor = tree_newnode(id);
  lor->narithbinary.left = n;
  lor->narithbinary.right = parse_arith_logical_or(p);

  return lor;
}
