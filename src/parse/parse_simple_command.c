#include "../parse.h"
#include "../tree.h"
#include "../fd.h"
#include "../sh.h"
#include "../debug.h"

/* 3.9.1 - parse a simple command
 * ----------------------------------------------------------------------- */
union node*
parse_simple_command(struct parser* p) {
  union node **aptr, *args;
  union node **vptr, *vars;
  union node **rptr, *rdir;
  union node* simple_command;
  size_t n = 0;

  tree_init(args, aptr);
  tree_init(vars, vptr);
  tree_init(rdir, rptr);

  for(;;) {
    /* look for assignments only when we have no args yet */
    switch(parse_gettok(p, P_DEFAULT)) {
      /* handle variable assignments */
      case T_ASSIGN:
        if(!(p->flags & P_NOASSIGN)) {
          *vptr = parse_getarg(p);
          vptr = &(*vptr)->list.next;
          break;
        }

      /* handle arguments */
      case T_NAME:
      case T_WORD:
        *aptr = parse_getarg(p);

        /* first argument */
        if(!(p->flags & P_NOASSIGN)) {
          struct nargstr* argstr = &(*aptr)->narg.list->nargstr;
          struct alias* a;

          if((a = parse_findalias(p, argstr->stra.s, argstr->stra.len))) {
            size_t codelen;
            struct source src;
            struct fd fd;
            struct parser aliasp;
            union node* node;
            char* code = alias_code(a, &codelen);

            source_buffer(&src, &fd, code, codelen);
            parse_init(&aliasp, P_ALIAS);
            aliasp.alias = a;

            node = parse_simple_command(&aliasp);

            buffer_puts(fd_err->w, "alias list ");
            debug_node(node, 0);

            source_popfd(&fd);

            // debug_node(*aptr, 0);

            if(node && node->id == N_SIMPLECMD) {
              struct ncmd* simple_cmd = &node->ncmd;
              tree_free(*aptr);

              vptr = tree_append(vptr, simple_cmd->vars);
              rptr = tree_append(rptr, simple_cmd->rdir);
              aptr = tree_append(aptr, simple_cmd->args);

              shell_free(simple_cmd);
            }
          }
        }

        if(*aptr)
          aptr = &(*aptr)->list.next;

        p->flags |= P_NOASSIGN;
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
    // p->flags |= P_NOKEYWD;
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

#ifdef DEBUG_OUTPUT
  if(sh->flags & SH_DEBUG) {
    buffer_puts(fd_err->w, "\x1b[1;33mparse_simple_command\x1b[0m = ");
    tree_print(simple_command, fd_err->w);
    buffer_putnlflush(fd_err->w);
  }
#endif

  return simple_command;
}
