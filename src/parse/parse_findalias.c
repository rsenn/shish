#include "../parse.h"

struct alias* parse_aliases = 0;

/* ----------------------------------------------------------------------- */
struct alias*
parse_findalias(const char* name, size_t len) {
  struct alias* a;

  for(a = parse_aliases; a; a = a->next) {
    if(a->len == len && byte_equal(a->name, len, name))
      return a;
  }
}
