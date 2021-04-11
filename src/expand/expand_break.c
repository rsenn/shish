#include "../expand.h"

union node**
expand_break(union node** out) {
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
