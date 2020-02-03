#include "builtin.h"
#include "exec.h"
#include "str.h"
#include "vartab.h"
#include <unistd.h>


/* hashed command search routine
 * ----------------------------------------------------------------------- */
union command
exec_hash(char* name, enum hash_id* idptr) {
  enum hash_id id;
  union command cmd;

  /* name contains slashes, its a path */
  if(name[str_chr(name, '/')]) {
    /* do not hash this... */
    id = H_PROGRAM;
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
    if((entry = exec_search(name, &hash))) {
      entry->hits++;
      id = entry->id;
      cmd = entry->cmd;
    }
    /* if we don't have a cache hit we're gonna search, special builtins first */
    else {
      id = H_EXEC;
      cmd.builtin = builtin_search(name, B_EXEC);

      /* then search for functions */
      if(cmd.builtin == NULL) {
        id = H_SBUILTIN;
        cmd.builtin = builtin_search(name, B_SPECIAL);
      }

      /* then search for functions */
      if(cmd.builtin == NULL) {
        id = H_FUNCTION;
        cmd.fn = /* FIXME */ NULL;
      }

      /* then search for normal builtins */
      if(cmd.fn == NULL) {
        id = H_BUILTIN;
        cmd.builtin = builtin_search(name, B_DEFAULT);
      }

      /* then search for external commands */
      if(cmd.builtin == NULL) {
        id = H_PROGRAM;
        cmd.path = exec_path(name);
      }

      /* if we found something then create a new cache entry */
      if(cmd.ptr) {
        entry = exec_create(name, hash);
        entry->hits++;
        entry->id = id;
        entry->cmd = cmd;
      }
    }
  }

  /* we have a command, set the id */
  if(cmd.ptr && idptr)
    *idptr = id;

  return cmd;
}
