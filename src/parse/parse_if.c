#include "parse.h"
#include "tree.h"

/* 3.9.4.4 - parse if conditional
 * ----------------------------------------------------------------------- */
union node*
parse_if(struct parser* p) {
  union node* node;
  union node** nptr;

  tree_init(node, nptr);

  /* parse if and elif statements in a loop */
  do {
    /* create new N_IF node and parse the test expression */
    *nptr = tree_newnode(N_IF);
    (*nptr)->nif.test = parse_compound_list(p);

    /* next token must be a T_THEN */
    if(!parse_expect(p, P_DEFAULT, T_THEN, node))
      return NULL;

    /* parse the tree for the first command */
    (*nptr)->nif.cmd0 = parse_compound_list(p);

    /* start a new branch on the else-case */
    tree_init((*nptr)->nif.cmd1, nptr);
  } while(parse_gettok(p, P_DEFAULT) == T_ELIF);

  /* check if we have an else-block, parse it if so */
  if(p->tok == T_ELSE)
    *nptr = parse_compound_list(p);
  else
    p->pushback++;

  /* next token must be a T_FI */
  if(!parse_expect(p, P_DEFAULT, T_FI, node))
    return NULL;

  return node;
}
