#include "../parse.h"
#include "../tree.h"

void
parse_newnode(struct parser* p, enum kind nod) {
  if(p->tree) {
    p->node->next = tree_newnode(nod);
    p->node = p->node->next;
  } else {
    p->node = tree_newnode(nod);
    p->tree = p->node;
  }
}
