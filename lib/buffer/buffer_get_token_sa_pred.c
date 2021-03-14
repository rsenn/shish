#include "../buffer.h"
#include "../byte.h"
#include "../stralloc.h"
#include <errno.h>

int
buffer_get_token_sa_pred(buffer* b, stralloc* sa, sa_predicate p, void* arg) {
  for(;;) {
    char x;
    if(!stralloc_readyplus(sa, 1))
      return -1;
    switch(buffer_getc(b, &x)) {
      case -1: return -1;
      case 0: return 0;
    }
    stralloc_append(sa, &x);
    switch(p(sa, arg)) {
      case -1: return -1;
      case 0: break;
      case 1: return 0;
    }
  }
}
