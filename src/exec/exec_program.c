#include "../../lib/alloc.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_ALLOCA
#include <alloca.h>
#endif

#include <errno.h>
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
#include "../../lib/stralloc.h"

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
  /* declared at function scope, not block scope: the child path below
     falls through past the closing brace of the "if" below and into
     fdtable_exec()/fdstack_flatten(), which still reference this frame
     via the global fdstack pointer fdstack_push() sets -- it was never
     popped in the child (only the parent branch pops it). Scoping it
     to the "if" block let the compiler treat that as a use-after-scope
     even though the child's use was intentional. */
  struct fdstack io;

  /* if we're gonna execve() a program and 'exec' isn't
     set or we aren't in the root shell environment we
     have to fork() so we can return */
  if(!(flag & X_EXEC) || sh->parent) {
    pid_t pid;
    struct fd* pipes = 0;
    unsigned int npipes;

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
       when the child does. SIGCHLD in particular has to be blocked
       before fork(): otherwise a fast-exiting child can deliver it (and
       have it handled -> job_signal() -> job_bypid() finding nothing)
       before job_new() below has even registered the job. */
#if !WINDOWS_NATIVE
    sig_block(SIGINT);
    sig_block(SIGCHLD);
#endif

    /* in the parent wait for the child to finish and then return
       or exit, according to the 'exec' argument */
    if((pid = fork())) {
      int status = 1;

      /* give the child its own process group (same double-setpgid-in-
         both-parent-and-child dance job_fork() already does, to avoid
         a race against whichever of the two runs first) -- without
         this, a backgrounded external command ("cmd &", the common
         case this branch handles) never gets a real process group of
         its own: job->pgrp below records 'pid' as if it were one, but
         killpg(job->pgrp, ...) (fg/bg's SIGCONT) and job_wait()'s own
         wait_pid(-job->pgrp, ...) both then target a process group
         that doesn't exist (ESRCH), silently doing nothing */
#if !WINDOWS_NATIVE
      setpgid(pid, pid);
#endif

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
        job->procs[0].status = -1;
        job->pgrp = pid;
        job->bgnd = 1;

        /* "$!" -- this backgrounds via its own raw fork() above
           instead of going through job_fork() (which sets this too),
           so it needs to be set here as well */
        job_bgpid = pid;

        job_banner(job, fd_err->w, JOB_START);

#if !WINDOWS_NATIVE
        sig_unblock(SIGCHLD);
#endif
        ret = 0;
      } else {
        struct job* job = job_new(1);

        job->procs[0].pid = pid;
        job->procs[0].status = -1;
        job->pgrp = pid;
        job->bgnd = 0;

        /* unlike a job_fork()-based backgrounded command, nothing
           downstream of this raw fork() ever gets a chance to set
           job->command from the parsed command line (that happens in
           eval_simple_command.c, but only for "ncmd->bgnd", and only
           by reading back *job_pointer -- which for a foreground job
           that already ran to completion no longer even points at
           this job, since job_wait() below just freed it). Build a
           display string from argv directly instead, so a "Stopped"/
           "jobs" line for a foreground command doesn't just say
           "(null)". */
        {
          stralloc sa;
          int k;

          stralloc_init(&sa);

          for(k = 0; argv[k]; k++) {
            if(k)
              stralloc_catc(&sa, ' ');

            stralloc_cats(&sa, argv[k]);
          }

          stralloc_nul(&sa);
          job->command = sa.s;
        }

        /* mirror job_fork()'s own bookkeeping for a foreground job:
           the child's own tcsetpgrp() call above already handed the
           terminal to its new process group, but job_wait()'s
           end-of-function "hand the terminal back to us" check
           compares against this global -- without updating it here
           too, that check never notices anything changed and the
           terminal stays with the now-exited child's defunct
           process group forever, wedging the shell's own next
           terminal read behind a SIGTTIN. This is also what makes a
           plain foreground command (no "&") resumable via fg/bg at
           all if it gets stopped (Ctrl-Z): before this, only a
           job_fork()-based path (i.e. one that started out
           backgrounded) ever created a struct job, so a foreground
           command's stop was never recorded anywhere to resume. */
        if(pid != job_pgrp) {
#if !WINDOWS_NATIVE
          if(fd_ok(job_terminal))
            tcsetpgrp(job_terminal, pid);
#endif
          job_pgrp = pid;
        }

        job_wait(job, 0, &status);

        ret = WEXITSTATUS(status);

        sig_blocknone();
      }

      /* exit if 'exec' is set, otherwise return */
      if((flag & X_EXEC))
        sh_exit(ret);

      return ret;
    }

    /* ...in the child we always exit */
    sh_forked();

#if !WINDOWS_NATIVE
    /* see the matching setpgid() call in the parent branch above */
    setpgid(sh_pid, sh_pid);

    /* a foreground external command should own the terminal while it
       runs, same as job_fork() already arranges for the job_fork()-
       based path -- a backgrounded one (X_NOWAIT) must not, it isn't
       supposed to be fighting the shell for input */
    if(!(flag & X_NOWAIT) && fd_ok(job_terminal))
      tcsetpgrp(job_terminal, sh_pid);
#endif

    /* the blocked mask set above survives exec(); the program we're
       about to run (or exit() out of) must not inherit SIGINT/SIGCHLD
       blocked */
#if !WINDOWS_NATIVE
    sig_blocknone();
#endif
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

    /* execve() returned so it failed. Save errno immediately: the
       buffer_puts()/write() calls inside sh_error_errno() below are
       free to leave errno changed (POSIX only guarantees it on
       failure, not success), so exec_error()'s mapping needs its own
       copy rather than re-reading a possibly-clobbered global. */
    {
      int saved_errno = errno;

      /* yield an error message */
      sh_error_errno(path);

      /* map the error code to the appropriate POSIX exit status */
      errno = saved_errno;
      ret = exec_error();
    }
  }

  /* we never return at this point! */
  exit(ret);
}
