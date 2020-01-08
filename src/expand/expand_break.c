#include "../expand.h"

node_t**
expand_break(node_t** out) {
  struct narg* this = &(*out)->narg;

  if(this) {
    if(this->stra.len) {
      out = &this->next;
      *out = tree_newnode(N_ARG);
      this = &(*out)->narg;
      stralloc_init(&this->stra);
    }
  }
  return out;
}