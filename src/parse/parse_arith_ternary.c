#include "../parse.h"
#include "../source.h"
#include "../tree.h"

/* parse arithmetic conditional ("?:") expression
 * ----------------------------------------------------------------------- */
union node*
parse_arith_ternary(struct parser* p) {
  union node *cond, *ontrue, *onfalse, *node;
  char c;

  if((cond = parse_arith_binary(p, ARITH_PREC_TOP)) == NULL)
    return NULL;

  parse_skipspace(p);

  if(source_peek(&c) <= 0 || c != '?')
    return cond;

  parse_skip(p);
  parse_skipspace(p);

  /* the true-branch is a full expression (POSIX grammar allows
     assignments in it, e.g. "1 ? a = 2 : 3"), while the false-branch
     recurses back into parse_arith_ternary() itself so chained
     conditionals ("a?b:c?d:e") associate right-to-left like C. */
  if((ontrue = parse_arith_expr(p)) == NULL)
    return NULL;

  parse_skipspace(p);

  if(source_peek(&c) <= 0 || c != ':')
    return NULL;

  parse_skip(p);
  parse_skipspace(p);

  if((onfalse = parse_arith_ternary(p)) == NULL)
    return NULL;

  node = tree_newnode(A_TERNARY);
  node->narithternary.cond = cond;
  node->narithternary.ontrue = ontrue;
  node->narithternary.onfalse = onfalse;

  return node;
}
