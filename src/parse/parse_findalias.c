#include "../parse.h"
#include "../../lib/byte.h"
#include "../source.h"

struct alias* parse_aliases = 0;

/* finds the alias called name, unless that alias is being read right now
 * (an alias never expands inside its own replacement text)
 * ----------------------------------------------------------------------- */
struct alias*
parse_findalias(struct parser* p, const char* name, size_t len) {
  struct alias* a;

  for(a = parse_aliases; a; a = a->next)
    if(a->namelen == len && !byte_diff(a->def, len, name))
      return source_alias_active(a) ? NULL : a;

  return NULL;
}
