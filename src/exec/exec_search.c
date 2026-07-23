#include "../builtin.h"
#include "../exec.h"
#include "../tree.h"
#include "../../lib/alloc.h"
#include "../../lib/str.h"

union node* functions = NULL;

/* Set while a subshell is evaluating. eval_function checks this to suppress
   freeing of redefined function nodes: their bodies may still be referenced
   from the parent's snapshot, and freeing them would dangle on restore. */
int exec_subshell_depth = 0;

void
exec_functions_save(struct func_snapshot* snap) {
  union node* p;
  size_t i;

  exec_subshell_depth++;

  snap->n = 0;
  snap->nodes = NULL;

  for(p = functions; p; p = p->next)
    snap->n++;

  if(snap->n) {
    snap->nodes = alloc(snap->n * sizeof(union node*));

    for(p = functions, i = 0; p; p = p->next, i++)
      snap->nodes[i] = p;
  }
}

void
exec_functions_restore(struct func_snapshot* snap) {
  int i;
  struct exechash* e;
  size_t k;

  exec_subshell_depth--;

  /* relink the saved nodes in their original order. Anything the subshell
     prepended to functions becomes unreachable; the node memory leaks but is
     safe — the subshell may have cached pointers into those bodies. */
  for(k = 0; k + 1 < snap->n; k++)
    snap->nodes[k]->next = snap->nodes[k + 1];

  if(snap->n) {
    snap->nodes[snap->n - 1]->next = NULL;
    functions = snap->nodes[0];
  } else {
    functions = NULL;
  }

  if(snap->nodes)
    alloc_free(snap->nodes);

  /* Cache may hold H_FUNCTION entries pointing at bodies that are still
     reachable but no longer the "current" definition (e.g., subshell redefined
     a function we just restored). Force a re-lookup. */
  for(i = 0; i < EXEC_HASHSIZE; i++)
    for(e = exec_hashtbl[i]; e; e = e->next)
      if(e->cmd.id == H_FUNCTION)
        e->mask = -1;
}

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
