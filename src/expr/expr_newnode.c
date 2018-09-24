#include "expr.h"

struct nargarith*
expr_newnode(struct parser *p, enum op_id op) {
  struct nargarith* n;
  parse_newnode(p, N_ARGARITH);
  n = &p->node->nargarith;
  n->op = op;
  return n;
}

