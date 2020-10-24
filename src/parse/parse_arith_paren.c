#include "../parse.h"
#include "../source.h"
#include "../tree.h"

/* parse arithmetic parentheses expression
 * ----------------------------------------------------------------------- */
union node*
parse_arith_paren(struct parser* p) {
  char c;
  union node *node, **nptr;

  if(source_peek(&c) <= 0)
    return NULL;

  if(c != '(')
    return NULL;

  parse_skip(p);
  parse_skipspace(p);

  node = tree_newnode(A_PAREN);

  nptr = &node->nargarith.tree;

again:
  if((*nptr = parse_arith_expr(p)) == NULL)
    return NULL;

  nptr = &(*nptr)->nargarith.next;

  if(source_peek(&c) <= 0)
    return NULL;

  if(c == ',') {
    parse_skip(p);
    goto again;
  }

  if(c != ')') {
    parse_error(p, T_RP);
    tree_free(node);
    return NULL;
  }

  parse_skip(p);

  return node;
}
