#include "../parse.h"
#include "../tree.h"
#include "../debug.h"

/* 3.9.3 - Lists
 * ----------------------------------------------------------------------- */

/* A list is a sequence of one or more AND-OR-lists separated by the
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
  union node* list;
  union node** nptr;
  enum tok_flag tok;

  /* keep looking for and-or lists */
  tree_init(list, nptr);

  while((*nptr = parse_and_or(p))) {
    tok = parse_gettok(p, P_DEFAULT);

    /* <newline> terminates the list and eats the token */
    if(tok & T_NL)
      break;

    /* there must be & or ; after the and-or list,
       otherwise the list will be terminated */
    if(!(tok & (T_SEMI | T_BGND))) {
      p->pushback++;
      break;
    }

    /* & causes async exec of preceding and-or list */
    if(tok & T_BGND)
      (*nptr)->ngrp.bgnd = 1;

    /* now check for another and-or list */
    tree_skip(nptr);
  }

#if defined(DEBUG_OUTPUT) && defined(DEBUG_PARSE) && !defined(SHPARSE2AST)
  if(list && list->next) {
    buffer_puts(debug_output, COLOR_YELLOW "parse_list" COLOR_NONE " ");
    if(list->next) {
      buffer_puts(debug_output, "[");
      buffer_putulong(debug_output, tree_count(list));
      buffer_puts(debug_output, "] ");
    }
    buffer_puts(debug_output, "list = ");

    if(list->next)
      debug_list(list, 0);
    else
      debug_node(list, 1);

    debug_nl_fl();
  }
#endif

  return list;
}
