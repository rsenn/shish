#include "../parse.h"
#include "../../lib/byte.h"

struct alias* parse_aliases = 0;

/* ----------------------------------------------------------------------- */
struct alias*
parse_findalias(const char* name, size_t len) {
  struct alias* a;

  for(a = parse_aliases; a; a = a->next) {
    if(a->namelen == len && byte_equal(a->def, len, name))
      return a;
  }
}
