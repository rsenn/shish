#include "../builtin.h"
#include "../exec.h"
#include "../tree.h"
#include "../../lib/str.h"

union node* functions = NULL;

/* command search routine
 * ----------------------------------------------------------------------- */
struct command
exec_search(char* name, int mask) {
  struct command cmd = {0, {0}};

  cmd.id = H_EXEC;
  cmd.builtin = builtin_search(name, B_EXEC);

  /* then search for special builtins */
  if(!(mask & H_SBUILTIN) && cmd.builtin == NULL) {
    cmd.id = H_SBUILTIN;
    cmd.builtin = builtin_search(name, B_SPECIAL);
  }

  /* then search for functions */
  if((mask & H_FUNCTION) == 0 && cmd.builtin == NULL) {
    struct nfunc* fn;
    cmd.id = H_FUNCTION;

    for(fn = &functions->nfunc; fn; fn = fn->next) {
      if(!str_diff(name, fn->name))
        break;
    }
    cmd.fn = fn ? fn->body : 0;
  }

  /* then search for normal builtins */
  if(!(mask & H_BUILTIN) && cmd.fn == NULL) {
    cmd.id = H_BUILTIN;
    cmd.builtin = builtin_search(name, B_DEFAULT);
  }

  /* then search for external commands */
  if(cmd.builtin == NULL) {
    cmd.id = H_PROGRAM;
    cmd.path = exec_path(name);
  }

  return cmd;
}
