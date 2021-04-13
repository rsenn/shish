#include "../parse.h"
#include "../tree.h"
#include "../debug.h"

/* 3.9.3 - Lists
 * ----------------------------------------------------------------------- */

/* A cmds is a sequence of one or more AND-OR-lists separated by the
 * operators
 *
 *       ;    &
 *
 * and optionally terminated by
 *
 *       ;    &    <newline>
 *
 * ----------------------------------------------------------------------- */
union node*
parse_list(struct parser* p) {
  union node *cmds, *list, **nptr;
  enum tok_flag tok;

  /* keep looking for and-or lists */
  tree_init(cmds, nptr);

  while((*nptr = parse_and_or(p))) {
    tok = parse_gettok(p, P_DEFAULT);

    /* <newline> terminates the cmds and eats the token */
    if(tok & T_NL)
      break;

    /* there must be & or ; after the and-or cmds,
       otherwise the cmds will be terminated */
    if(!(tok & (T_SEMI | T_BGND))) {
      p->pushback++;
      break;
    }

    /* & causes async exec of preceding and-or cmds */
    if(tok & T_BGND)
      (*nptr)->ngrp.bgnd = 1;

    /* now check for another and-or cmds */
    tree_skip(nptr);
  }

  if(tree_count(cmds) > 1) {
    list = tree_newnode(N_LIST);

    list->nlist.cmds = cmds;
  } else {
    list = cmds;
  }

#if defined(DEBUG_OUTPUT) && defined(DEBUG_PARSE) && !defined(SHPARSE2AST)
  if(cmds && cmds->next) {
    buffer_puts(debug_output, COLOR_YELLOW "parse_list" COLOR_NONE " ");
    if(cmds->next) {
      buffer_puts(debug_output, "[");
      buffer_putulong(debug_output, tree_count(cmds));
      buffer_puts(debug_output, "] ");
    }
    buffer_puts(debug_output, "cmds = ");

    if(cmds->next)
      debug_list(cmds, 0);
    else
      debug_node(cmds, 1);

    debug_nl_fl();
  }
#endif

  return list;
}
