#include "../buffer.h"
#include "../stralloc.h"

int
buffer_get_new_token_sa_pred(buffer* b,
                             stralloc* sa,
                             sa_predicate p,
                             void* arg) {
  stralloc_zero(sa);
  return buffer_get_token_sa_pred(b, sa, p, arg);
}
