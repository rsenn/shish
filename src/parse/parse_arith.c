#include "tree.h"
#include "parse.h"
#include "source.h"

/* parse arithmetic expression
 * ----------------------------------------------------------------------- */
int parse_arith(struct parser *p)
{
  union node *tree;
  struct parser subp;
  char c;

  source_skip();
  
  parse_init(&subp, P_ARITH);
 
  tree = parse_arith_expr(&subp);

  do {
  if(!source_get(&c)) return -1;
  } while(isspace(c));
  if(c != ')') {
    return -1;
  }
  if(!source_get(&c)) return -1;
  if(c != ')') {
    return -1;
  }

  parse_newnode(p, N_ARGARITH);
  p->node->nargarith.tree = tree;
  
  return 0;
}

