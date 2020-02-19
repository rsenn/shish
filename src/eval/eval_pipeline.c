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
  ssize_t n;
  int pid = 0;
  int prevfd = -1;
  int status = -1;

  /*  job = (e->flags & E_JCTL) ? job_new(npipe->ncmd) : NULL;*/
  job = job_new(npipe->ncmd);

  if(job == NULL) {
    buffer_puts(fd_err->w, "no job control");
    buffer_putnlflush(fd_err->w);
  }

#if DEBUG_OUTPUT
  for(node = npipe->cmds; node; node = node->list.next) {
    debug_node(node, 0);
    buffer_putnlflush(buffer_2);
  }
#endif

  for(node = npipe->cmds; node; node = node->list.next) {
    fdstack_push(&st);

    /* if there was a previous command we read input from pipe */
    if(prevfd >= 0) {
      struct fd* in;

#ifdef HAVE_ALLOCA
      in = fd_alloca();
      fd_push(in, STDIN_FILENO, FD_READ | FD_PIPE);
#else
      in = fd_malloc();
      fd_push(in, STDIN_FILENO, FD_READ | FD_PIPE | FD_FREE);
#endif
      fd_setfd(in, prevfd);
    }

    /* if it isn't the last command we have to create a pipe
       to pass output to the next command */
    if(node->list.next || (fdtable[STDOUT_FILENO]->mode & FD_SUBST) == FD_SUBST) {
      struct fd* out;

#ifdef HAVE_ALLOCA
      out = fd_alloca();
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
      int exitcode;
      /* no job control for commands inside pipe */
      /*      e->mode &= E_JCTL;*/
      exitcode = eval_tree(e, node, E_EXIT);

      /* exit after evaluating this subtree */
      exit(exitcode);
    } else {
      buffer_puts(buffer_2, "forked: ");
      buffer_putulong(buffer_2, pid);
      buffer_putnlflush(buffer_2);
    }

    /*

        if(!node->list.next) {
                  struct fd* fd = fdtable[1];
          if((fd->mode & FD_SUBST) == FD_SUBST) {
            char b[1024];
            int  e = fd->rb.fd;
            while((n = read(e, b, sizeof(b))) > 0) {
              buffer_put(fd->w, b, n);
            }
            buffer_flush(fd->w);
          }
        } */
    if(!node->list.next)
      fdstack_data();

    fdstack_pop(&st);
  }

  if(!npipe->bgnd) {
    job_wait(job, 0, &status);
  }

  if(pipes)
    shell_free(pipes);

  /*  if(job)
      shell_free(job);*/

  return WEXITSTATUS(status);
}
