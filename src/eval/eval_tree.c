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
  //  int oldflags;

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

  while(node) {
    /* not the last node, disable E_EXIT for now */
    if(ex && (!list || node->list.next == NULL))
      e->flags |= E_EXIT;
    ret = eval_node(e, node);

    if(!list)
      break;

    node = node->list.next;
  }

  if(ex)
    sh_exit(ret);
  return ret;
}