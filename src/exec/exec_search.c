#include "../exec.h"
#include "../../lib/str.h"

/* searches hashtable entry for <name>
 * ----------------------------------------------------------------------- */
struct exechash*
exec_search(char* name, uint32* hashptr) {
  uint32 hash = exec_hashstr(name);
  struct exechash* entry;

  for(entry = exec_hashtbl[hash & EXEC_HASHMASK]; entry; entry = entry->next) {
    if(entry->hash == hash) {
      if(!str_diff(entry->name, name)) {
        if(hashptr)
          *hashptr = hash;
        return entry;
      }
    }
  }

  return NULL;
}
