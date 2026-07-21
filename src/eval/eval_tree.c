#include "../fd.h"
#include "../fdtable.h"
#include "../eval.h"
#include "../job.h"
#include "../tree.h"
#include "../sh.h"

/* evaluate a tree node(-list maybe)
 * ----------------------------------------------------------------------- */
int
eval_tree(struct eval* e, union node* node, int tempflags) {
  int ret = 0;
  int list = 0, ex = 0;
  // int oldflags;

  if((e->flags | tempflags) & E_LIST) {
    list = 1;
    e->flags &= ~E_LIST;
    tempflags &= ~E_LIST;
  }

  if((e->flags | tempflags) & E_EXIT) {
    ex = 1;
    e->flags &= ~E_EXIT;
    tempflags &= ~E_EXIT;
  }

  // oldflags = e->flags;
  e->flags |= tempflags;

  pid_t pid;
  struct job* job = 0;

  while(node) {
    /* not the last node, disable E_EXIT for now */
    if(ex && (!list || node->next == NULL))
      e->flags |= E_EXIT;

    /* a backgrounded compound command ("{ cmd; } &", "(cmd) &", ...)
       has to fork and return immediately, exactly like a backgrounded
       simple command (see eval_simple_command.c's X_NOWAIT path) --
       not run again by the parent afterwards, and not waited for
       synchronously (that would defeat backgrounding it at all) */
    if(node->id != N_SIMPLECMD && node->ncmd.bgnd) {
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

      ret = 0;
    } else {
      ret = eval_node(e, node);
    }

    e->exitcode = ret;

    if(!list)
      break;

    node = node->next;
  }

  if(ex)
    sh_exit(ret);

  return ret;
}
