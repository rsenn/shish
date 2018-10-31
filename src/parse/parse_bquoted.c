#include "tree.h"
#include "parse.h"
#include "source.h"
#include "expand.h"

/* parse backquoted commands
 * ----------------------------------------------------------------------- */
int parse_bquoted(struct parser *p) {
  char c;
  union node *cmds;
  struct parser subp;

  if(p->tok == T_NAME)
    p->tok = T_WORD;

  if(source_peek(&c) <= 0)
    return -1;
  
  if(c == '(') {
    if(source_next(&c) <= 0)
      return -1;
/*
    if(c == '(')
      return parse_arith(p);*/
    
    parse_init(&subp, P_DEFAULT);
  } else {
    source_skip();
    subp.flags = P_BQUOTE;
    
    parse_init(&subp, P_BQUOTE);
  }

  if((cmds = parse_compound_list(&subp)) == NULL)
    return -1;

  /* MUST be terminated with right parenthesis or backquote */
  if(!parse_expect(&subp, P_DEFAULT, ((subp.flags & P_BQUOTE) ? T_BQ : T_RP), cmds))
    return -1;

  parse_newnode(p, N_ARGCMD);
  p->node->nargcmd.flag = ((subp.flags & P_BQUOTE) ? S_BQUOTE : 0) | p->quot;
  p->node->nargcmd.list = cmds;
  
  return 0;
}

