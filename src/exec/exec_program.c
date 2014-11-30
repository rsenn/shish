#include "../../config.h"

#ifdef HAVE_ALLOCA
#include <alloca.h>
#endif

#ifndef WIN32
#include <unistd.h>
#endif
#include <signal.h>
#include "tree.h"
#include "parse.h"
#include "fdstack.h"
#include "fdtable.h"
#include "redir.h"
#include "exec.h"
#include "job.h"
#include "var.h"
#include "sh.h"

/* execute another program, possibly searching for it first
 * 
 * if the 'exec' argument is set it will never return
 * ----------------------------------------------------------------------- */
int exec_program(char *path, char **argv, int exec, union node *redir)
{
  int ret = 0;
  sigset_t nset, oset;
  
  /* if we're gonna execve() a program and 'exec' isn't 
     set or we aren't in the root shell environment we
     have to fork() so we can return */
  if(!exec || sh->parent)
  {
    pid_t pid;
    struct fdstack io;
    unsigned int n;

    fdstack_push(&io);

    /* buffered fds which have not a real effective file descriptor,
       like here-docs which are read from strallocs and command
       expansions, which write to strallocs can't be shared across
       different process spaces, so we have to establish pipes */
    if((n = fdstack_npipes(FD_HERE|FD_SUBST)))
      fdstack_pipe(n, fdstack_alloc(n));

    /* block child and interrupt signal, so we won't terminate ourselves
       when the child does */
    sigemptyset(&nset);
    sigaddset(&nset, SIGINT);
    sigaddset(&nset, SIGCHLD);
//    sigemptyset(&oset);
    sigprocmask(SIG_BLOCK, &nset, &oset);
    
    /* in the parent wait for the child to finish and then return 
       or exit, according to the 'exec' argument */
    if((pid = fork()))
    {
      int status = 1;

      /* this will close child ends of the pipes and read data from the parent end :) */
      fdstack_pop(&io);
      fdstack_data();

      
      job_wait(NULL, pid, &status, 0);
      job_status(pid, status);

      ret = WEXITSTATUS(status);

      sigprocmask(SIG_SETMASK, &oset, NULL);
      
      /* exit if 'exec' is set, otherwise return */
      if(exec) sh_exit(ret);
      return ret;
    }

    /* ...in the child we always exit */
    sh_forked();
  }

  fdtable_exec();
  fdstack_flatten();

  /* when there is a path then we gotta execute a command,
     otherwise we exit/return immediately */
  if(path)
  {
    /* export environment */
    char **envp;
    unsigned long envn = var_count(V_EXPORT) + 1;
    envp = var_export(alloca(envn * sizeof(char *)));

    /* try to execute the program */
    execve(path, argv, envp);

    /* execve() returned so it failed, we're gonna map 
       the error code to the appropriate POSIX errors */
    ret = exec_error();

    /* yield an error message */
    sh_error(path);
  }

  /* we never return at this point! */
  exit(ret);
}
