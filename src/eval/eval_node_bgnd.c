#include "../fd.h"
#include "../fdtable.h"
#include "../eval.h"
#include "../job.h"
#include "../tree.h"
#include "../sh.h"

/* evaluate a node the same way eval_node() does, except that a
 * backgrounded compound command ("{ cmd; } &", "(cmd) &", ...) forks
 * and returns immediately instead of running in-process, exactly like
 * a backgrounded simple command already does (see
 * eval_simple_command.c's X_NOWAIT path). Every caller that walks a
 * chain of sibling nodes (a plain command list, or the pipeline-free
 * and-or-list chain built by parse_list()) needs this instead of a
 * bare eval_node() call, or a bgnd compound sitting anywhere but the
 * chain's sole node silently runs synchronously.
 * ----------------------------------------------------------------------- */
int
eval_node_bgnd(struct eval* e, union node* node) {
  int ret;
  pid_t pid;
  struct job* job;

  if(node->id == N_SIMPLECMD || !node->ncmd.bgnd)
    return eval_node(e, node);

  job = job_new(1);
  pid = job_fork(job, 0, 1);

  if(!pid) {
    ret = eval_node(e, node);
    exit(ret);
  }

  buffer_putc(fd_err->w, '[');
  buffer_putulong(fd_err->w, job->id);
  buffer_puts(fd_err->w, "] ");
  buffer_putulong(fd_err->w, pid);
  buffer_putnlflush(fd_err->w);

  return 0;
}
