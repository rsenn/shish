#include "../parse.h"
#include "../tree.h"

#ifdef DEBUG_ALLOC

void
parse_newnodedebug(const char* file,
                   unsigned int line,
                   struct parser* p,
                   enum nod_id nod) {
  if(p->tree) {
    p->node->list.next = tree_newnodedebug(file, line, nod);
    p->node = p->node->list.next;
  } else {
    p->node = tree_newnodedebug(file, line, nod);
    p->tree = p->node;
  }
}
#endif /* defined(DEBUG_ALLOC) */