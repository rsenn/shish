#include "config.h"

#ifdef HAVE_ALLOCA
#include <alloca.h>
#endif

<<<<<<< HEAD
#ifndef WIN32
#include <unistd.h>
#endif
#ifndef WIN32
=======
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#ifndef __MINGW32__
>>>>>>> 6c7455723b47a4989fb5bb621be8f200a306f361
#include <sys/wait.h>
#endif
#include "fdstack.h"
#include "sh.h"
#include "job.h"
#include "vartab.h"
#include "tree.h"
#include "parse.h"
#include "eval.h"
#include "exec.h"
#include "redir.h"
#include "expand.h"

/* evaluate a simple command (3.9.1)
 * 
 * this function doesn't put stuff in background, it always wait()s, so
 * it only needs to fork() real programs
 * ----------------------------------------------------------------------- */
int eval_simple_command(struct eval *e, struct ncmd *ncmd)
{
  union node *nptr;
  int argc;
  char **argv;
  int status;
  union node *args = NULL;
  union node *assigns = NULL;
  union command cmd = { NULL };
  enum hash_id id = H_BUILTIN;
  struct vartab vars;
//  struct fdstack io;
  union node *r;
  union node *redir = ncmd->rdir;

  /* expand arguments,
     if there are arguments we start a hashed search for the command */
  if(expand_args(ncmd->args, &args, 0))
  {
    stralloc_nul(&args->narg.stra);
    cmd = exec_hash(args->narg.stra.s, &id);
  }

  /* expand and set the variables,
     mark them for export if we're gonna execute a command */
  if(expand_vars(ncmd->vars, &assigns))
  {
    /* if we don't exit after the command, have a command and not a 
       special builtin the variable changes should be temporary */
    if(!(e->flags & E_EXIT) && cmd.ptr && id != H_SBUILTIN)
      vartab_push(&vars);
    
    for(nptr = assigns; nptr; nptr = nptr->list.next)
      var_setsa(&nptr->narg.stra, (cmd.ptr ? V_EXPORT : V_DEFAULT));

    tree_free(assigns);
  }
  
  /* do redirections if present */
/*  if(redir && id != H_SBUILTIN && id != H_EXEC)
    fdstack_push(&io);*/
    
  if(redir/* && id != H_PROGRAM*/)
  {
    for(r = redir; r; r = r->list.next)
    {
      struct fd *fd = NULL;
      
      /* if its the exec special builtin the new fd needs to be persistent */
      if(id != H_EXEC) fd_alloca(fd);
      
      /* return if a redirection failed */
      if(redir_eval(&r->nredir, fd, (id == H_EXEC ? R_NOW : 0)))
      {
        status = 1;
        goto end;
      }
      
      /* check if we need to initialize fd buffers for the new redirection */
      if(fd_needbuf(r->nredir.fd))
      {
        /* if its not exec then set up buffers for 
           temporary redirections on the stack */
        if(id != H_EXEC)
          fd_setbuf(r->nredir.fd, alloca(FD_BUFSIZE), FD_BUFSIZE);
        else
          fd_allocbuf(r->nredir.fd, FD_BUFSIZE);
      }
    }
  }
  
  /* if there is no command we can return after 
     setting the vars and doing the redirections */
  if(args == NULL)
  {
    status = 0;
    goto end;
  }

  /* when the command wasn't found we abort */
  if(cmd.ptr == NULL)
  {
    sh_error(args->narg.stra.s);
    status = exec_error();
    goto end;
  }

  /* assemble argument list */
  argc = tree_count(args);
  
  argv = alloca((argc + 1) * sizeof(char *));
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
  if(id != H_EXEC)
  {
    for(r = redir; r; r = r->list.next)
      fd_pop(r->nredir.fd);
  }
  
/*  if(fdstack == &io)
    fdstack_pop(&io);*/

  return status;
}

