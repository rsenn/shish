#include "../parse.h"
#include "../tree.h"

/* 3.9.4.5
 * 3.9.4.6 - parse while/until loops
 * ----------------------------------------------------------------------- */
union node*
parse_loop(struct parser* p) {
  union node* node;

  /* create list node and parse test expression */
  node = tree_newnode((p->tok == T_WHILE) ? N_WHILE : N_UNTIL);

  /* there must be newline or semicolon after the test expression */
  node->nloop.test = parse_compound_list(p, T_DO);

  /* ..and then a "do" must follow */
  if(!parse_expect(p, P_DEFAULT, T_DO, node))
    return NULL;

  /* now parse the loop body */
  node->nloop.cmds = parse_compound_list(p, T_DONE);

  /* ..and then a "done" must follow */
  if(!parse_expect(p, P_DEFAULT, T_DONE, node))
    return NULL;

  return node;
}
