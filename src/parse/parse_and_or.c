#include "../parse.h"
#include "../tree.h"

/* parse a boolean AND-OR list
 *
 * An AND-OR-list is a sequence of one or more pipelines separated by the
 * operators
 *
 *        &&    ||
 *
 * !!! may return NULL when there are no commands
 * ----------------------------------------------------------------------- */
union node*
parse_and_or(struct parser* p) {
  union node *pipeline0, *pipeline1, *and_or;
  enum tok_flag tok;

  /* parse a command or a pipeline first */
  pipeline0 = parse_pipeline(p);

  while(pipeline0) {
    tok = parse_gettok(p, P_DEFAULT);

    /* whether && nor ||, it's not a list, return the pipeline */
    if(!(tok & (T_AND | T_OR))) {
      p->pushback++;
      break;
    }

    /* there can be a newline after the operator but this isn't
       mentioned in the draft text, only in the parser grammar */
    while(parse_gettok(p, P_SKIPNL) & T_NL)
      ;
    p->pushback++;

    /* try to parse another pipeline */
    if((pipeline1 = parse_pipeline(p)) == NULL) {
      p->pushback++;
      break;
    }

    /* set up a nandor node and continue */
    and_or = tree_newnode(tok == T_AND ? N_AND : N_OR);
    and_or->nandor.left = pipeline0;
    and_or->nandor.right = pipeline1;
    pipeline0 = and_or;
  }

  return pipeline0;
}
