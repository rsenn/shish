#include "parse.h"
#include "source.h"
#include "tree.h"

/* parse arithmetic binary expression
 * ----------------------------------------------------------------------- */
union node*
parse_arith_binary(struct parser* p, int mult) {
  union node *lnode, *rnode, *newnode;
  char c;
  int ntype = -1;

  lnode = mult ? parse_arith_unary(p) : parse_arith_binary(p, 1);

  if(lnode == NULL)
    return NULL;

  parse_skipspace(p);

  if(source_peek(&c) <= 0) {
    tree_free(lnode);
    return NULL;
  }

  if(mult) {
    switch(c) {
      case '*': ntype = N_ARITH_MUL; break;
      case '/': ntype = N_ARITH_DIV; break;
    }
  } else {
    switch(c) {
      case '+': ntype = N_ARITH_ADD; break;
      case '-': ntype = N_ARITH_SUB; break;
    }
  }

  if(ntype == -1)
    return lnode;

  source_skip();
  parse_skipspace(p);

  rnode = mult ? parse_arith_unary(p) : parse_arith_binary(p, 1);

  if(rnode == NULL) {
    tree_free(lnode);
    return NULL;
  }

  newnode = tree_newnode(ntype);
  newnode->narithbinary.left = lnode;
  newnode->narithbinary.right = rnode;

  return newnode;
}
