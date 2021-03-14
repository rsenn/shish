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

struct nfunc* functions = NULL;

/* hashed command search routine
 * ----------------------------------------------------------------------- */
union command
exec_hash(char* name, int mask) {
  union command cmd = {0, {0}};

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
    if((entry = exec_search(name, &hash))) {
      entry->hits++;
      cmd = entry->cmd;
    }
    /* if we don't have a cache hit we're gonna search, special builtins first
     */
    else {
      cmd.id = H_EXEC;
      cmd.builtin = builtin_search(name, B_EXEC);

      /* then search for functions */
      if(cmd.builtin == NULL) {
        cmd.id = H_SBUILTIN;
        cmd.builtin = builtin_search(name, B_SPECIAL);
      }

      /* then search for functions */
      if(!(mask & H_FUNCTION) && cmd.builtin == NULL) {
        struct nfunc* fn;
        cmd.id = H_FUNCTION;

        for(fn = functions; fn; fn = fn->next) {
          if(!str_diff(name, fn->name))
            break;
        }
        cmd.fn = fn ? fn->body : 0;
      }

      /* then search for normal builtins */
      if(cmd.fn == NULL) {
        cmd.id = H_BUILTIN;
        cmd.builtin = builtin_search(name, B_DEFAULT);
      }

      /* then search for external commands */
      if(cmd.builtin == NULL) {
        cmd.id = H_PROGRAM;
        cmd.path = exec_path(name);
      }

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
