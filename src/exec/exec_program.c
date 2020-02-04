#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_ALLOCA
#include <alloca.h>
#endif

#include <signal.h>
#include "../../lib/windoze.h"
#include "../exec.h"
#include "../fdstack.h"
#include "../fdtable.h"
#include "../job.h"
#include "../parse.h"
#include "../redir.h"
#include "../sh.h"
#include "../sig.h"
#include "../tree.h"
#include "../var.h"
#include "../../lib/wait.h"

#if WINDOWS_NATIVE
pid_t fork(void);
#endif

/* execute another program, possibly searching for it first
 *
 * if the 'exec' argument is set it will never return
 * ----------------------------------------------------------------------- */
int
exec_program(char* path, char** argv, int exec, union node* redir) {
  int ret = 0;
  struct fd* pipes = 0;
  sigset_t /* nset, */ oset;

  /* if we're gonna execve() a program and 'exec' isn't
     set or we aren't in the root shell environment we
     have to fork() so we can return */
  if(!exec || sh->parent) {
    pid_t pid;
    struct fdstack io;
    unsigned int n;

    fdstack_push(&io);

    /* buffered fds which have not a real effective file descriptor,
       like here-docs which are read from strallocs and command
       expansions, which write to strallocs can't be shared across
       different process spaces, so we have to establish pipes */
    if((n = fdstack_npipes(D_HERE | D_SUBST))) {
      pipes = shell_alloc(FDSTACK_ALLOC_SIZE(n));
      fdstack_pipe(n, pipes);
    }

    /* block child and interrupt signal, so we won't terminate ourselves
       when the child does */
    /*
    sigemptyset(&nset);
    sigaddset(&nset, SIGINT);

#ifdef SIGCHLD
    sigaddset(&nset, SIGCHLD);
#endif
    sigemptyset(&oset);
    sigprocmask(SIG_BLOCK, &nset, &oset);
*/
    sig_block();

    /* in the parent wait for the child to finish and then return
       or exit, according to the 'exec' argument */
    if((pid = fork())) {
      int status = 1;

      /* this will close child ends of the pipes and read data from the parent end :) */
      fdstack_pop(&io);
      fdstack_data();

    if(pipes)
      shell_free(pipes);

      job_wait(NULL, pid, &status);
      job_status(pid, status);

      ret = WAIT_EXITSTATUS(status);

#if !WINDOWS_NATIVE
      sigprocmask(SIG_SETMASK, &oset, NULL);
#endif

      /* exit if 'exec' is set, otherwise return */
      if(exec)
        sh_exit(ret);

      return ret;
    }

    /* ...in the child we always exit */
    sh_forked();
  }

  fdtable_exec();
  fdstack_flatten();

  /* when there is a path then we gotta execute a command,
     otherwise we exit/return immediately */
  if(path) {
    /* export environment */
    char** envp;
    unsigned long envn = var_count(V_EXPORT) + 1;
    envp = var_export(shell_alloc(envn * sizeof(char*)));

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
