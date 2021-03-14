#include "../parse.h"
#include "../../lib/byte.h"

struct alias* parse_aliases = 0;

static inline int
alias_match(struct alias* a, const char* name, size_t namelen) {
  return a->namelen == namelen && byte_equal(a->def, namelen, name);
}

/* ----------------------------------------------------------------------- */
struct alias*
parse_findalias(struct parser* p, const char* name, size_t len) {
  struct alias* a;

  for(a = parse_aliases; a; a = a->next) {
    if((p->flags & P_ALIAS) && alias_match(a, name, len))
      continue;
    if(a->namelen == len && !byte_diff(a->def, len, name))
      break;
  }
  return a;
}
