#include "../expand.h"
#include "../parse.h"
#include "../source.h"
#include "../tree.h"

/* parse backquoted commands
 * ----------------------------------------------------------------------- */
int
parse_bquoted(struct parser* p) {
  char c;
  union node* cmds;
  struct parser subp;
  enum tok_flag end_tok;

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
    end_tok = T_RP;

    parse_init(&subp, P_DEFAULT);
  } else {
    parse_skip(p);
    subp.flags = P_BQUOTE;
    end_tok = T_BQ;

    parse_init(&subp, P_BQUOTE);
  }

  if((cmds = parse_compound_list(&subp, end_tok)) == NULL)
    return -1;
  /*
  subp.pushback++;
  if(subp.tok == T_BQ)
   parse_gettok(&subp, 0); */

  /* MUST be terminated with right parenthesis or backquote */
  /*   if(!parse_expect(&subp, P_DEFAULT, end_tok, cmds))
      return -1; */

  parse_newnode(p, N_ARGCMD);
  p->node->nargcmd.flag = ((subp.flags & P_BQUOTE) ? S_BQUOTE : 0) | p->quot;
  p->node->nargcmd.list = cmds;

  return 0;
}
