#include "tree.h"
#include "parse.h"

#ifndef DEBUG_ALLOC

void parse_newnode(struct parser *p, enum nod_id nod) {
  if(p->tree) {
    p->node->list.next = tree_newnode(nod);
    p->node = p->node->list.next;
  } else {
    p->node = tree_newnode(nod);
    p->tree = p->node;
  }
}
#endif /* !defined(DEBUG_ALLOC) */