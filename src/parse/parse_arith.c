#include "tree.h"
#include "parse.h"
#include "source.h"

/* parse arithmetic expression
 * ----------------------------------------------------------------------- */
int parse_arith(struct parser *p)
{
  union node *tree;
  struct parser subp;

  source_skip();
  
  parse_init(&subp, P_ARITH);

 


  if((tree = parse_compound_list(&subp)) == NULL)
    return -1;

  /* MUST be terminated with right parenthesis or backquote */
  if(!parse_expect(&subp, P_DEFAULT, T_RP, tree) ||
     !parse_expect(&subp, P_DEFAULT, T_RP, tree))
    return -1;

  parse_newnode(p, N_ARGARITH);
  p->node->nargarith.tree = tree;
  
  return 0;
}

