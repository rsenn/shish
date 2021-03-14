#include "../parse.h"
#include "../tree.h"
#include "../fd.h"
#include "../sh.h"
#include "../debug.h"
#include "../source.h"

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
          stralloc sa;
          struct nargstr* argstr = &(*aptr)->narg.list->nargstr;
          struct alias* a;

          stralloc_init(&sa);
          stralloc_copy(&sa, &argstr->stra);

          if((a = parse_findalias(p, sa.s, sa.len))) {
            size_t codelen;
            struct source src;
            struct fd fd;
            struct parser aliasp;
            union node* node;
            char* code;

            code = alias_code(a, &codelen);
            source_buffer(&src, &fd, code, codelen);
            parse_init(&aliasp, P_ALIAS);
            aliasp.alias = a;

            if((node = parse_simple_command(&aliasp))) {
              // debug_node(node, 0);

              tree_remove(aptr);

              vptr = tree_append(vptr, node->ncmd.vars);
              rptr = tree_append(rptr, node->ncmd.rdir);
              aptr = tree_append(aptr, node->ncmd.args);
              shell_free(node);
            }

            source_popfd(&fd);
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

#ifdef DEBUG_PARSE
  if(sh->flags & SH_DEBUG) {
    buffer_puts(fd_err->w, COLOR_YELLOW "parse_simple_command" COLOR_NONE " = ");
    tree_print(simple_command, fd_err->w);
    buffer_putnlflush(fd_err->w);
  }
#endif

  return simple_command;
}
