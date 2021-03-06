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
#include "../debug.h"
#include "../../lib/windoze.h"
#include "../../lib/str.h"
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
  int status = 0;
  union node* args = NULL;
  union node* assigns = NULL;
  struct command cmd = {H_BUILTIN, {0}};
  // enum hash id = H_BUILTIN;
  struct vartab vars;
  struct fdstack io;
  union node* r;
  union node* redir = ncmd->rdir;
  char buf[FD_BUFSIZE];

  /* expand arguments,
     if there are arguments we start a hashed search for the command */
  if(expand_args(ncmd->args, &args, 0)) {
    stralloc_nul(&args->narg.stra);
    cmd = exec_hash(args->narg.stra.s, 0);
  }

  /* expand and set the variables,
     mark them for export if we're gonna execute a command */
  if(expand_vars(ncmd->vars, &assigns)) {
    /* if we don't exit after the command, have a command and not a
       special builtin the variable changes should be temporary */
    if(!(e->flags & E_EXIT) && cmd.ptr && cmd.id != H_SBUILTIN)
      vartab_push(&vars, 0);

    for(nptr = assigns; nptr; nptr = nptr->list.next) {
      if(!var_setsa(&nptr->narg.stra, (cmd.ptr ? V_EXPORT : V_DEFAULT))) {
        status = 1;
        break;
      }
    }

    tree_free(assigns);

    if(status)
      goto end;
  }

  /* do redirections if present */
  if(redir && cmd.id != H_SBUILTIN && cmd.id != H_EXEC)
    fdstack_push(&io);

  if(redir /* && cmd.id != H_PROGRAM*/) {
    for(r = redir; r; r = r->nredir.next) {
      struct fd* fd = NULL;

      /* if its the exec special builtin the new fd needs to be persistent */
      if(cmd.id != H_EXEC) {
#ifdef HAVE_ALLOCA
        fd = fd_alloc();
#else
        fd = fd_malloc();
#endif
      }

      /* return if a redirection failed */
      if(redir_eval(&r->nredir, fd, (cmd.id == H_EXEC ? R_NOW : 0))) {
        status = 1;
        goto end;
      }

      /* check if we need to initialize fd buffers for the new redirection */
      if(fd_needbuf(r->nredir.fd)) {
        /* if its not exec then set up buffers for
           temporary redirections on the stack */
        if(cmd.id != H_EXEC)
          fd_setbuf(r->nredir.fd, buf, FD_BUFSIZE);
        else
          fd_allocbuf(r->nredir.fd, FD_BUFSIZE);
      }

#if DEBUG_OUTPUT_
      buffer_puts(&debug_buffer, "Redirection ");
      debug_node(r, -1);
      if(r->nredir.fd) {
        buffer_puts(&debug_buffer, "fd { n= ");
        buffer_putlong(&debug_buffer, r->nredir.fd->n);
        buffer_puts(&debug_buffer, " }");
      }
      debug_nl_fl();
#endif
    }
  }

  /* if there is no command we can return after
     setting the vars and doing the redirections */
  if(args == NULL)
    goto end;

  /* when the command wasn't found we abort */
  if(cmd.ptr == NULL) {
    sh_error(args->narg.stra.s);
    status = exec_error();
    goto end;
  }

  /* assemble argument list */
  argc = tree_count(args);
#ifdef HAVE_ALLOCA
  argv = alloca((argc + 1) * sizeof(char*));
#else
  argv = shell_alloc((argc + 1) * sizeof(char*));
#endif
  expand_argv(args, argv);

  if(sh->opts.debug) {
    char** arg;
    buffer_puts(fd_err->w, "+");
    for(arg = argv; *arg; arg++) {
      int quote = !!(*arg)[str_chr(*arg, ' ')];
      buffer_putspace(fd_err->w);
      if(quote)
        buffer_putc(fd_err->w, '\'');
      buffer_puts(fd_err->w, *arg);
      if(quote)
        buffer_putc(fd_err->w, '\'');
    }
    buffer_putnlflush(fd_err->w);
  }

  /* execute the command, this may or may not return, depending on E_EXIT */
  status = exec_command(&cmd, argc, argv, (e->flags & E_EXIT), redir);

#ifndef HAVE_ALLOCA
  shell_free(argv);
#endif
end:

  /* restore variable stack */
  if(varstack == &vars)
    vartab_pop(&vars);

  if(args)
    tree_free(args);

  /* undo redirections */

  if(fdstack == &io)
    fdstack_pop(&io);
  else if(cmd.id != H_EXEC) {
    for(r = redir; r; r = r->nredir.next) {
      fd_pop(r->nredir.fd);
    }
  }

  sh->exitcode = status;

  return status;
}
