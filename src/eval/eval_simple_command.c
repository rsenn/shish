#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef HAVE_ALLOCA
#include <alloca.h>
#endif
#include "../fd.h"
#include "../sh.h"
#include "../eval.h"
#include "../exec.h"
#include "../expand.h"
#include "../fdstack.h"
#include "../job.h"
#include "../parse.h"
#include "../redir.h"
#include "../tree.h"
#include "../vartab.h"
#include "../../lib/windoze.h"
#if !WINDOWS_NATIVE
#include <sys/wait.h>
#include <unistd.h>
#endif

/* evaluate a simple command (3.9.1)
 *
 * this function doesn't put stuff in background, it always wait()s, so
 * it only needs to fork() real programs
 * ----------------------------------------------------------------------- */
int
eval_simple_command(struct eval* e, struct ncmd* ncmd) {
  union node* nptr;
  int argc;
  char** argv;
  int status;
  union node* args = NULL;
  union node* assigns = NULL;
  union command cmd = {NULL};
  enum hash_id id = H_BUILTIN;
  struct vartab vars;
  /*  struct fdstack io;*/
  union node* r;
  union node* redir = ncmd->rdir;
  char buf[D_BUFSIZE];

  /* expand arguments,
     if there are arguments we start a hashed search for the command */
  if(expand_args(ncmd->args, &args, 0)) {
    stralloc_nul(&args->narg.stra);
    cmd = exec_hash(args->narg.stra.s, &id);
  }

  /* expand and set the variables,
     mark them for export if we're gonna execute a command */
  if(expand_vars(ncmd->vars, &assigns)) {
    /* if we don't exit after the command, have a command and not a
       special builtin the variable changes should be temporary */
    if(!(e->flags & E_EXIT) && cmd.ptr && id != H_SBUILTIN)
      vartab_push(&vars);

    for(nptr = assigns; nptr; nptr = nptr->list.next) var_setsa(&nptr->narg.stra, (cmd.ptr ? V_EXPORT : V_DEFAULT));

    tree_free(assigns);
  }

  /* do redirections if present */
  /*  if(redir && id != H_SBUILTIN && id != H_EXEC)
      fdstack_push(&io);*/

  if(redir /* && id != H_PROGRAM*/) {
    for(r = redir; r; r = r->list.next) {
      struct fd* fd = NULL;

      /* if its the exec special builtin the new fd needs to be persistent */
      if(id != H_EXEC) {
#ifdef HAVE_ALLOCA
        fd_alloca(fd);
#endif
      }

      /* return if a redirection failed */
      if(redir_eval(&r->nredir, fd, (id == H_EXEC ? R_NOW : 0))) {
        status = 1;
        goto end;
      }

      /* check if we need to initialize fd buffers for the new redirection */
      if(fd_needbuf(r->nredir.fd)) {
        /* if its not exec then set up buffers for
           temporary redirections on the stack */
        if(id != H_EXEC)
          fd_setbuf(r->nredir.fd, buf, D_BUFSIZE);
        else
          fd_allocbuf(r->nredir.fd, D_BUFSIZE);
      }
    }
  }

  /* if there is no command we can return after
     setting the vars and doing the redirections */
  if(args == NULL) {
    status = 0;
    goto end;
  }

  /* when the command wasn't found we abort */
  if(cmd.ptr == NULL) {
    sh_error(args->narg.stra.s);
    status = exec_error();
    goto end;
  }

  /* assemble argument list */
  argc = tree_count(args);

  argv = alloca((argc + 1) * sizeof(char*));
  expand_argv(args, argv);

  /* execute the command, this may or may not return, depending on E_EXIT */
  status = exec_command(id, cmd, argc, argv, (e->flags & E_EXIT), redir);

end:

  /* restore variable stack */
  if(varstack == &vars)
    vartab_pop(&vars);

  if(args)
    tree_free(args);

  /* undo redirections */
  if(id != H_EXEC) {
    for(r = redir; r; r = r->list.next) fd_pop(r->nredir.fd);
  }

  /*  if(fdstack == &io)
      fdstack_pop(&io);*/

  return status;
}
