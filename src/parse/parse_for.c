#include "tree.h"
#include "parse.h"

/* 3.9.4.2 - parse for loop
 * ----------------------------------------------------------------------- */
union node*
parse_for(struct parser *p) {
  union node *node = NULL;
  union node **nptr;

  /* next token must be the variable name */
  if(!parse_expect(p, P_DEFAULT, T_NAME, node))
    return NULL;

  /* allocate and init N_FOR node */
  node = tree_newnode(N_FOR);

  stralloc_nul(&p->node->nargstr.stra);
  node->nfor.varn = p->node->nargstr.stra.s;
  stralloc_init(&p->node->nargstr.stra);
  
  tree_init(node->nfor.args, nptr);

  /* next token can be 'in' */
  if(parse_gettok(p, P_DEFAULT) & T_IN) {
    /* now parse the arguments and build a list of them */
    while(parse_gettok(p, P_DEFAULT) & (T_WORD|T_NAME|T_ASSIGN)) {
      *nptr = parse_getarg(p);
      nptr = &(*nptr)->list.next;
    }
  }
  p->pushback++;

  /* there can be a semicolon after the argument list */
  if(!(parse_gettok(p, P_DEFAULT) & T_SEMI))
    p->pushback++;

  /* ..and the next token must be the "do" keyword */
  if(!parse_expect(p, P_SKIPNL, T_DO, node))
    return NULL;

  /* parse the commands inside "do"<->"done" */
  node->nfor.cmds = parse_compound_list(p);

  /* next token must be the "done" keyword */
  if(!parse_expect(p, P_DEFAULT, T_DONE, node))
    return NULL;

  return node;
}

