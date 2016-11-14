#ifndef __MINGW32__
# include <unistd.h>
# include <sys/wait.h>
#endif
#include "fdstack.h"
#include "fdtable.h"
#include "tree.h"
#include "eval.h"
#include "job.h"
#include "sh.h"

/* evaluate a pipeline (3.9.2)
 * ----------------------------------------------------------------------- */
int eval_pipeline(struct eval *e, struct npipe *npipe) {
  struct job *job;
  union node *node;
  struct fdstack st;
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
  
  
  for(node = npipe->cmds; node; node = node->list.next) {
    fdstack_push(&st);
    
    /* if there was a previous command we read input from pipe */
    if(prevfd >= 0) {
      struct fd *in;
      
      fd_alloca(in);
      fd_push(in, STDIN_FILENO, FD_READ|FD_PIPE);
      fd_setfd(in, prevfd);
    }
    
    /* if it isn't the last command we have to create a pipe
       to pass output to the next command */
    if(node->list.next) {
      struct fd *out;
      
      fd_alloca(out);
      fd_push(out, STDOUT_FILENO, FD_WRITE|FD_PIPE);
      prevfd = fd_pipe(out);
      
      if(prevfd == -1) {
        close(prevfd);
        sh_error("pipe creation failed");
      }
    }

    if((n = fdstack_npipes(FD_HERE|FD_SUBST)))
      fdstack_pipe(n, fdstack_alloc(n));
    
    pid = job_fork(job, node, npipe->bgnd);
    
    if(!pid) {
      /* no job control for commands inside pipe */
/*      e->mode &= E_JCTL;*/
      
      /* exit after evaluating this subtree */
      exit(eval_tree(e, node, E_EXIT));
    }
    
    fdstack_pop(&st);
    fdstack_data();
  }
  
  if(!npipe->bgnd) {
    job_wait(job, 0, &status, 0);
  }

/*  if(job)
    shell_free(job);*/
 
	return WEXITSTATUS(status);
}
