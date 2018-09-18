#include "expr.h"

struct nargexpr*
expr_newnode(struct parser *p, enum op_id op) {
  struct nargexpr* n;
  parse_newnode(p, N_ARGEXPR);
  n = &p->node->nargexpr;
  n->op = op;
  return n;
}

