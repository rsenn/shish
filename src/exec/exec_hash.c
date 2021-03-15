#include "../builtin.h"
#include "../exec.h"
#include "../tree.h"
#include "../../lib/str.h"
#include "../vartab.h"
#include "../../lib/windoze.h"
#if WINDOWS_NATIVE
#include <io.h>
#ifndef X_OK
#define X_OK 1
#endif
#else
#include <unistd.h>
#endif

/* hashed command search routine
 * ----------------------------------------------------------------------- */
struct command
exec_hash(char* name, int mask) {
  struct command cmd = {0, {0}};

  /* name contains slashes, its a path */
  if(name[str_chr(name, '/')]) {
    /* do not hash this... */
    cmd.id = H_PROGRAM;
    cmd.path = name;

    /* ...but validate the path */
    if(access(cmd.path, X_OK) != 0)
      cmd.path = NULL;
  }
  /* otherwise try to find hashed entry */
  else {
    struct exechash* entry;
    uint32 hash;

    /* hash the name for possible re-use on exechash_create() */
    hash = exec_hashstr(name);

    /* do we have a cache hit? */
    if((entry = exec_lookup(name, &hash))) {
      entry->hits++;
      cmd = entry->cmd;
    } else {
      /* if we don't have a cache hit we're gonna search, special builtins first */
      cmd = exec_search(name, mask);

      /* if we found something then create a new cache entry */
      if(cmd.ptr) {
        entry = exec_create(name, hash);
        entry->hits++;
        entry->cmd = cmd;
      }
    }
  }
  return cmd;
}
