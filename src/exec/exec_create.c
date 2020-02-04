#include "../../lib/byte.h"
#include "exec.h"
#include "sh.h"
#include "../../lib/shell.h"
#include <string.h>

struct exechash* exec_hashtbl[EXEC_HASHSIZE];

/* generates new hashtable entry for <name>
 * ----------------------------------------------------------------------- */
struct exechash*
exec_create(char* name, uint32 hash) {
  struct exechash* entry;

  if((entry = shell_alloc(sizeof(struct exechash)))) {
    byte_zero(entry, sizeof(struct exechash));
    entry->hash = hash;
    entry->name = shell_strdup(name);
    entry->next = exec_hashtbl[entry->hash & EXEC_HASHMASK];
    exec_hashtbl[entry->hash & EXEC_HASHMASK] = entry;
  }
  return entry;
}
