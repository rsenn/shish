#include "../../lib/alloc.h"
#include "../../lib/str.h"
#include "../tree.h"
#include "../eval.h"
#include "../exec.h"
#include "../sh.h"

extern union node* functions;

static inline union node**
find_function(const char* name) {
  union node** nptr = &functions;

  for(nptr = &functions; *nptr; nptr = tree_next(nptr)) {
    struct nfunc* fn = &(*nptr)->nfunc;

    if(!str_diff(fn->name, name))
      return nptr;
  }

  return 0;
}

/* Walk a node tree and hash all command names when hashall (-h) is enabled.
 * This implements the "hash on define" behavior required by POSIX.
 */
static void
hash_commands_in_node(union node* node) {
  if(!node || !sh->opts.hashall)
    return;

  switch(node->id) {
    case N_SIMPLECMD:
      /* Simple command - hash the first argument (command name) */
      if(node->ncmd.args) {
        union node* arg = node->ncmd.args;
        if(arg->id == N_ARG && arg->narg.list && arg->narg.list->id == N_ARGSTR) {
          char* name = arg->narg.list->nargstr.stra.s;
          if(name && name[0] && !str_chr(name, '/')) {
            /* Only hash if it's a simple name (not a path) */
            exec_hash(name, 0);
          }
        }
      }
      /* Recurse into redirections and other arguments */
      hash_commands_in_node(node->ncmd.rdir);
      break;

    case N_PIPELINE:
      /* Pipeline - hash each command in the pipeline */
      {
        union node* cmd;
        for(cmd = node->npipe.cmds; cmd; cmd = cmd->next)
          hash_commands_in_node(cmd);
      }
      break;

    case N_AND:
    case N_OR:
      hash_commands_in_node(node->nandor.left);
      hash_commands_in_node(node->nandor.right);
      break;

    case N_LIST:
      hash_commands_in_node(node->nlist.cmds);
      hash_commands_in_node(node->nlist.rdir);
      break;

    case N_SUBSHELL:
    case N_BRACEGROUP:
      hash_commands_in_node(node->ngrp.cmds);
      hash_commands_in_node(node->ngrp.rdir);
      break;

    case N_FOR:
      hash_commands_in_node(node->nfor.cmds);
      break;

    case N_CASE:
      {
        union node* pat;
        for(pat = node->ncase.list; pat; pat = pat->next)
          hash_commands_in_node(pat->ncasenode.cmds);
      }
      break;

    case N_IF:
      hash_commands_in_node(node->nif.test);
      hash_commands_in_node(node->nif.cmd0);
      hash_commands_in_node(node->nif.cmd1);
      break;

    case N_WHILE:
    case N_UNTIL:
      hash_commands_in_node(node->nloop.test);
      hash_commands_in_node(node->nloop.cmds);
      break;

    default:
      break;
  }

  /* Walk the next pointer for lists */
  if(node->id != N_PIPELINE && node->id != N_CASE)
    hash_commands_in_node(node->next);
}

/* ----------------------------------------------------------------------- */
int
eval_function(struct eval* e, struct nfunc* func) {
  int ret = 0;
  union node *fn, **nptr;

  /* When hashall (-h) is enabled, hash commands in the function body
   * at definition time (POSIX "hash on define" behavior) */
  if(sh->opts.hashall && func->body)
    hash_commands_in_node(func->body);

  if((nptr = find_function(func->name))) {
    fn = *nptr;
    *nptr = (*nptr)->next;
    /* IMPORTANT: detach fn from the rest of the list before freeing.
       tree_free walks via node->next, so without this the call below
       would cascade and free every function that was defined earlier
       than this one (as_fn_exit, as_fn_unset, ... all gone when
       configure redefines as_fn_nop). */
    fn->next = NULL;
    /* While a subshell is evaluating, the parent shell's func_snapshot may
       still reference this node. Leak it for now (orphaned but safe);
       exec_functions_restore will relink the parent's list on exit. */
    if(exec_subshell_depth == 0)
      tree_free(fn);
  }

  /* Invalidate any cached exec_hash entry for this name. The cache holds
     cmd.fn = body pointer, which we either just freed (redefine) or are
     about to replace. Without this, autoconf's `. ./$as_me.lineno` self-
     source resets all functions but the cache still serves the stale body
     pointer, producing a use-after-free on the next invocation. */
  {
    uint32 h;
    struct exechash* e = exec_lookup(func->name, &h);
    if(e)
      e->mask = -1; /* force exec_search re-run on next lookup */
  }

  fn = tree_newnode(N_FUNCTION);

  /* Deep-copy the name/body into the "functions" list entry instead of
     stealing (moving, then NULLing) the pointers straight out of `func`.
     Stealing only works if this N_FUNCTION node is evaluated exactly
     once before its enclosing statement is freed (sh_loop.c frees each
     top-level statement right after running it) -- it breaks the moment
     the same node is evaluated again, e.g. a function defined inside a
     loop body or a repeatedly-invoked in-process subshell (see
     eval_subshell.c's exec_functions_save/restore, which deliberately
     discards subshell-installed definitions when the subshell scope
     ends, so the definition must be reinstallable on every visit). A
     second visit found func->name/func->body already NULLed out from
     the first, and the exec_lookup() call above dereferenced the NULL
     name directly -> segfault. tree_copy() gives every installed
     definition its own independent copy, so `func` is left untouched
     and can be evaluated any number of times. Confirmed via a real
     crash: looping "f() { :; }" inside a while loop segfaulted in
     exec_hashstr() on a NULL name within a few iterations. */
  fn->next = functions;
  fn->nfunc.name = str_dup(func->name);
  fn->nfunc.body = func->body ? tree_copy(func->body) : NULL;

  functions = fn;

  return ret;
}
