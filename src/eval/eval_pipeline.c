#include "../../lib/alloc.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "../fd.h"
#include "../sh.h"
#include "../eval.h"
#include "../fdstack.h"
#include "../fdtable.h"
#include "../job.h"
#include "../tree.h"
#include "../debug.h"
#include "../../lib/windoze.h"
#if !WINDOWS_NATIVE
#include <sys/wait.h>
#include <unistd.h>
#endif

/* evaluate a pipeline (3.9.2)
 * ----------------------------------------------------------------------- */
int
eval_pipeline(struct eval* e, struct npipe* npipe) {
  struct job* job;
  union node* node;
  struct fdstack st;
  struct fd* pipes = 0;
  unsigned int n;
  int pid = 0;
  int prevfd = -1;
  int status = -1;

  /*  job = (e->flags & E_JCTL) ? job_new(npipe->ncmd) : NULL;*/
  job = job_new(npipe->ncmd);

  if(job)
    job->bgnd = npipe->bgnd;

  if(job == NULL) {
    buffer_puts(fd_err->w, "no job control");
    buffer_putnlflush(fd_err->w);
  }

  fdstack_push(&st);

  for(node = npipe->cmds; node; node = node->next) {
    struct fd *in = 0, *out = 0;

    /* if there was a previous command we read input from pipe */
    if(prevfd >= 0) {

#ifdef HAVE_ALLOCA
      in = fd_alloc();
      fd_push(in, STDIN_FILENO, FD_READ | FD_PIPE);
#else
      in = fd_malloc();
      fd_push(in, STDIN_FILENO, FD_READ | FD_PIPE | FD_FREE);
#endif
      fd_setfd(in, prevfd);
    }

    /* if it isn't the last command we have to create a pipe
       to pass output to the next command */
    if(node->next /* || (fd_out->mode & FD_SUBST) == FD_SUBST */) {

#ifdef HAVE_ALLOCA
      out = fd_alloc();
      fd_push(out, STDOUT_FILENO, FD_WRITE | FD_PIPE);
#else
      in = fd_malloc();
      fd_push(out, STDOUT_FILENO, FD_WRITE | FD_PIPE | FD_FREE);
#endif
      prevfd = fd_pipe(out);

      if(prevfd == -1) {
        /* prevfd is already -1 here; close(-1) is a no-op that only
           risks clobbering errno (with EBADF) before it's reported */
        sh_error_errno("pipe creation failed");
      }
    }

    /* fdstack_npipes()/fdstack_pipe() wire a real pipe for a
       command-substitution target found in the fdstack (fd_subst()
       only sets up an in-process stralloc sink, nothing a forked
       child can write into -- and job_fork() always forks, even for
       a builtin). This only makes sense for the *last* pipeline
       member: it's the only one whose stdout the substitution target
       actually cares about, and calling it for an earlier member
       would hijack that member's stdout away from the inter-stage
       pipe ("out" above) that's supposed to feed the next member's
       stdin instead. exec_program.c uses this same pair for the
       (pipeline-free) command-substitution case. */
    if(!node->next && (n = fdstack_npipes(FD_HERE | FD_SUBST))) {
      pipes = alloc(FDSTACK_ALLOC_SIZE(n));
      fdstack_pipe(n, pipes);
    }

    pid = job_fork(job, node, npipe->bgnd);

    if(!pid) {
      /* no job control for commands inside pipe */
      /*      e->mode &= E_JCTL;*/

      /* exit after evaluating this subtree */
      exit(eval_tree(e, node, E_EXIT));
    } else {
#ifdef DEBUG_OUTPUT
      debug_ulong("forked", pid, 0);
#endif
    }

    if(!node->next && pipes) {
      unsigned int i;

      /* the pipe write-end(s) fdstack_pipe() created above are only
         needed by the child we just forked into -- close our own
         (parent-side) copies before draining, or fdstack_data()'s
         read() would never see EOF (our own open copy would keep the
         pipe writable forever). Matches exec_program.c's
         fdstack_pop(&io) (which is fdstack_pop(&st) below, but that
         has to wait until every pipeline member has run) followed by
         fdstack_data() -- the same shared drain used there, so a
         command substitution nested inside this pipeline's own
         members gets read back correctly too, not just this one. */
      for(i = 0; i < n; i++)
        fd_pop(&pipes[i]);

      fdstack_data();
    }

    if(out)
      fd_pop(out);

    if(in)
      fd_pop(in);
  }

  fdstack_pop(&st);

  if(!npipe->bgnd) {
    job_wait(job, 0, &status);
  }

  if(pipes)
    alloc_free(pipes);

  if(job)
    job_free(job);

  return WEXITSTATUS(status);
}
