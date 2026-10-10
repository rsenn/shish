#include "../parse.h"
#include "../trace.h"
#include "../tree.h"
#include "../fd.h"
#include "../sh.h"
#include "../source.h"
#include <assert.h>

/* 3.9.1 - parse a simple command
 * ----------------------------------------------------------------------- */
union node*
parse_simple_command(struct parser* p) {
  union node **aptr, *args, **vptr, *vars, **rptr, *rdir, *simple_command;
  size_t n = 0;
  struct location pos;

  pos = p->tokstart;

  tree_init(args, aptr);
  tree_init(vars, vptr);
  tree_init(rdir, rptr);

  for(;;) {
    enum tok_flag tok = parse_gettok(p, P_DEFAULT);

    /* look for assignments only when we have no args yet */
    switch(tok) {
      /* handle variable assignments */
      case T_ASSIGN:
        if(!(p->flags & P_NOASSIGN)) {
          *vptr = parse_getarg(p);
          tree_skip(vptr);
          break;
        }

      /* handle arguments */
      case T_NAME:
      case T_WORD:
        *aptr = parse_getarg(p);

        if(*aptr)
          tree_skip(aptr);

        p->flags |= P_NOASSIGN | P_NOKEYWD;
        break;

      /* handle redirections */
      case T_REDIR:
        tree_move(p->tree, rptr);
        break;

        /* end of command */
      default: {
        p->pushback++;
        goto addcmd;
      }
    }

    /* after the first word token we do not longer
       scan for keywords because a simple command
       ends with a control operator */
    p->flags &= ~P_SKIPNL;
    n++;
  }

addcmd:
  p->flags &= ~(P_NOKEYWD | P_NOASSIGN);

  /* add a command node */
  simple_command = tree_newnode(N_SIMPLECMD);

  /*  node->ncmd.bgnd = 0; not done here in posix*/
  simple_command->ncmd.args = args;
  simple_command->ncmd.vars = vars;
  simple_command->ncmd.rdir = rdir;

  TRACE(TRACE_PARSE, "simple_command", trace_loc("loc", &pos), trace_node("text", simple_command));

  return simple_command;
}
