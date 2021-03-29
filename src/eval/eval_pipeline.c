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
        close(prevfd);
        sh_error("pipe creation failed");
      }
    }

    if((n = fdstack_npipes(FD_HERE | FD_SUBST))) {
      pipes = shell_alloc(FDSTACK_ALLOC_SIZE(n));
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

    if(!node->next) {
      if((fd_out->mode & FD_STRALLOC)) {
        int fd = fd_out->parent->rb.fd;
        ssize_t r;
        char b[1024];
        while((r = read(fd, b, sizeof(b))) > 0) {
          buffer_put(fd_out->w, b, r);
        }
      }
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
    shell_free(pipes);

  /*  if(job)
      shell_free(job);*/

  return WEXITSTATUS(status);
}
