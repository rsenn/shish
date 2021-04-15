#include "../../lib/alloc.h"
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
#include "../tree.h"
#include "../var.h"
#include "../debug.h"
#include "../../lib/sig.h"
#include "../../lib/wait.h"

#if WINDOWS_NATIVE
pid_t fork(void);
#else
#include <unistd.h>
#endif

/* execute another program, possibly searching for it first
 *
 * if the 'exec' argument is set it will never return
 * ----------------------------------------------------------------------- */
int
exec_program(char* path, char** argv, enum execflag flag) {
  int ret = 0;

  /* if we're gonna execve() a program and 'exec' isn't
     set or we aren't in the root shell environment we
     have to fork() so we can return */
  if(!(flag & X_EXEC) || sh->parent) {
    pid_t pid;
    struct fd* pipes = 0;
    unsigned int npipes;
    struct fdstack io;

    fdstack_push(&io);

    /* buffered fds which have not a real effective file descriptor,
       like here-docs which are read from strallocs and command
       expansions, which write to strallocs can't be shared across
       different process spaces, so we have to establish pipes */
    if((npipes = fdstack_npipes(FD_HERE | FD_SUBST))) {
      pipes = alloc(FDSTACK_ALLOC_SIZE(npipes));
      fdstack_pipe(npipes, pipes);
    }

#if defined(DEBUG_OUTPUT) && defined(DEBUG_FDTABLE)
    // fdstack_dump(debug_output);
    fdtable_dump(debug_output);
    // fd_dumplist(debug_output);
#endif

    /* block child and interrupt signal, so we won't terminate ourselves
       when the child does */
#if !WINDOWS_NATIVE
    sig_block(SIGINT);
    sig_block(SIGCHLD);
#endif

    /* in the parent wait for the child to finish and then return
       or exit, according to the 'exec' argument */
    if((pid = fork())) {
      int status = 1;

      /* this will close child ends of the pipes and read data from the parent
       * end :) */
      fdstack_pop(&io);
      if(npipes)
        fdstack_data();

      if(pipes)
        alloc_free(pipes);

      if(flag & X_NOWAIT) {
        struct job* job = job_new(1);

        job->procs[0].pid = pid;

        buffer_putc(fd_err->w, '[');
        buffer_putulong(fd_err->w, job->id);
        buffer_puts(fd_err->w, "] ");
        buffer_putulong(fd_err->w, pid);
        buffer_putnlflush(fd_err->w);

        ret = 0;
      } else {
        job_wait(NULL, pid, &status);
        job_status(pid, status);

        ret = WAIT_EXITSTATUS(status);

        sig_blocknone();
      }

      /* exit if 'exec' is set, otherwise return */
      if((flag & X_EXEC))
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
    envp = var_export(alloc(envn * sizeof(char*)));

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
