#include "../parse.h"
#include "../tree.h"
#include "../debug.h"
#include "../fd.h"

/* 3.9.4.1 parse a grouping compound
 *
 * The format for grouping commands is a s follows:
 *
 *  (compound-list)       Execute compound-list in a subshell environment;
 *                        Variable assignments and built-in commands that
 *                        affect the environment shall not remain in effect
 *                        after the list finishes.
 *
 *  { compound-list;}     Execute compound-list in the current process
 *                        environment.
 *
 * ----------------------------------------------------------------------- */
union node*
parse_grouping(struct parser* p, int tempflags) {
  enum tok_flag tok;
  union node** rptr;
  union node* grouping;
  union node* compound_list;

  /* return NULL on empty compound */
  grouping = NULL;

  if(!(tok = parse_expect(p, P_DEFAULT | tempflags, T_BEGIN | T_LP, NULL)))
    return NULL;

  /* parse compound content and create a
     compound node if there are commands */
  if((compound_list = parse_compound_list(p, tok << 1))) {
    grouping = tree_newnode(tok == T_BEGIN ? N_BRACEGROUP : N_SUBSHELL);
    grouping->ngrp.cmds = compound_list;
  }

  /* expect the appropriate ending token */
  if(!parse_expect(p, P_DEFAULT, tok << 1, grouping))
    return NULL;

  if(grouping) {
#if defined(DEBUG_OUTPUT) && defined(DEBUG_PARSE) && !defined(SHFORMAT) && !defined(SHPARSE2AST)
    debug_node(grouping, -2);
    debug_nl_fl();
#endif
    tree_init(grouping->ngrp.rdir, rptr);

    /* now any redirections may follow */
    while(parse_gettok(p, P_DEFAULT) & T_REDIR) tree_move(p->tree, rptr);

    p->pushback++;
  }

  return grouping;
}
