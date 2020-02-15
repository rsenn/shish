#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "../fd.h"
#include "../eval.h"
#include "../fdstack.h"
#include "../parse.h"
#include "../redir.h"
#include "../tree.h"
#ifdef HAVE_ALLOCA
#include <alloca.h>
#endif

/* evaluate a compound or simple command
 *
 * those are all subject to redirections, therefore perform these before
 * launching the command
 * ----------------------------------------------------------------------- */
int
eval_command(struct eval* e, union node* node, int tempflags) {
  int ret = 1;
  int oldflags;
  struct fdstack fdstack;
  stralloc heredoc;
  char buf[FD_BUFSIZE];
  union node* redir = node->ncmd.rdir;

  oldflags = e->flags;
  e->flags |= tempflags;

  /* do redirections if present */
  if(redir) {
    union node* r;
    fdstack_push(&fdstack);
    stralloc_init(&heredoc);

    for(r = redir; r; r = r->list.next) {
      struct fd* fd = 0;
#ifdef HAVE_ALLOCA
      fd = fd_alloca();
#endif

      /* return if a redirection failed */
      if(redir_eval(&r->nredir, fd, R_NOW)) {
        ret = 1;
        goto fail;
      }

      if(fd_needbuf(fd))
        fd_setbuf(fd, buf, FD_BUFSIZE);
    }
  }

  switch(node->id) {
    case N_IF: ret = eval_if(e, &node->nif); break;
    case N_FOR: ret = eval_for(e, &node->nfor); break;
    case N_CASE: ret = eval_case(e, &node->ncase); break;
    case N_WHILE:
    case N_UNTIL: ret = eval_loop(e, &node->nloop); break;
    case N_SUBSHELL: ret = eval_subshell(e, &node->ngrp); break;
    case N_CMDLIST: ret = eval_cmdlist(e, node->ngrp.cmds); break;
    default:
      break;
      /*    case N_SIMPLECMD:
          case N_REDIR:
          case N_ASSIGN:
          default:
            ret = eval_simple_command(&node->ncmd);
            break;*/
  }

fail:
  e->flags = oldflags;

  /* undo redirections */
  if(redir)
    fdstack_pop(&fdstack);

  return ret;
}
