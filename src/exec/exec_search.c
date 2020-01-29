#include "exec.h"
#include "str.h"

/* searches hashtable entry for <name>
 * ----------------------------------------------------------------------- */
struct exechash*
exec_search(char* name, uint32* hashptr) {
  uint32 hash = *hashptr = exec_hashstr(name);
  struct exechash* entry;

  for(entry = exec_hashtbl[hash & EXEC_HASHMASK]; entry; entry = entry->next) {
    if(entry->hash == hash)
      if(!str_diff(entry->name, name))
        return entry;
  }

  return NULL;
}
