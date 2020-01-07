#include "parse.h"
#include "source.h"
#include "tree.h"

/* parse arithmetic binary expression
 * ----------------------------------------------------------------------- */
union node*
parse_arith_binary(struct parser* p) {
  union node *lnode, *rnode, *newnode;
  char c;
  int ntype = -1;

  lnode = parse_arith_value(p);

  if(lnode == NULL)
    return NULL;

  parse_skipspace(p);

  if(source_peek(&c) <= 0) {
    tree_free(lnode);
    return NULL;
  }

  switch(c) {
    case '+': ntype = N_ARITH_ADD; break;
    case '-': ntype = N_ARITH_SUB; break;
    case '*': ntype = N_ARITH_MUL; break;
    case '/': ntype = N_ARITH_DIV; break;
  }

  if(ntype == -1) {
    tree_free(lnode);
    return NULL;
  }

  source_skip();
  parse_skipspace(p);

  rnode = parse_arith_value(p);

  if(rnode == NULL) {
    tree_free(lnode);
    return NULL;
  }

  newnode = tree_newnode(ntype);
  newnode->narithbinary.left = lnode;
  newnode->narithbinary.right = rnode;

  return newnode;
}
