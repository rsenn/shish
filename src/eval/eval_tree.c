#include "../fd.h"
#include "../eval.h"
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

    if(node->id != N_SIMPLECMD)
      if(node->ncmd.bgnd) {
        job = job_new(1);

        if((pid = job_fork(job, 0, 1))) {
          ret = eval_node(e, node);
          exit(ret);
        }

        int st = 0;
        ret = job_wait(job, pid, &st);
      }

    ret = eval_node(e, node);
    e->exitcode = ret;

    if(!list)
      break;

    node = node->next;
  }

  if(ex)
    sh_exit(ret);

  return ret;
}
