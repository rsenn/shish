#include "../expand.h"
#include "../tree.h"
#include "../debug.h"
#include "../fd.h"
#include "../parse.h"
#include "../sh.h"

/* expand all arguments of an argument list
 * returns count of argument nodes
 * ----------------------------------------------------------------------- */
int expand_error = 0;

/* word is only empty literals and quoted plain $@, and there are no
 * positional parameters: "$@" -> zero fields, not one empty field
 * (""$@ and $@"" stay one empty field)
 * ----------------------------------------------------------------------- */
static int
expand_is_empty_at(union node* word) {
  union node* sub;
  int at = 0;

  if(sh->arg.c || !word || word->id != N_ARG)
    return 0;

  for(sub = word->narg.list; sub; sub = sub->next) {
    if(sub->id == N_ARGSTR && sub->nargstr.stra.len == 0)
      continue;

    if(sub->id == N_ARGPARAM && (sub->nargparam.flag & (S_SPECIAL | S_VAR)) == S_ARGVS &&
       (sub->nargparam.flag & S_TABLE) == S_DQUOTED) {
      at = 1;
      continue;
    }

    return 0;
  }

  return at;
}

int
expand_args(union node* args, union node** nptr, int flags) {
  union node* arg;
  union node* n;
  union node* owned;
  union node** head = nptr;
  int ret = 0, copied;

  *nptr = NULL;

  /* args is the permanent parsed command tree, reused on every
     execution -- brace/tilde expansion rewrite word text/structure,
     so they must only touch a private, disposable copy. Only words
     they might rewrite need one; expand_arg() itself never writes. */
  for(arg = args; arg; arg = arg->next)
    if(expand_brace_needed(arg) || expand_tilde_needed(arg))
      break;

  owned = arg ? expand_brace_args(tree_copy(args)) : args;
  copied = arg != NULL;

  for(arg = owned; arg; arg = arg->next) {

#ifdef DEBUG_OUTPUT_
    debug_node(arg, 0);
    debug_nl_fl();
#endif

    if(expand_is_empty_at(arg))
      continue;

    if(copied)
      expand_tilde_word(arg);

    if((n = expand_arg(arg->narg.list, nptr, flags))) {
      nptr = &n;
      ret++;
    }

    if(n == NULL)
      continue;

    if(n->narg.flag & X_GLOB) {
      if((n = expand_glob(nptr, n->narg.flag & ~X_GLOB))) {
        nptr = &n;
        ret++;
      }
    } else if(n->narg.flag & X_GLOBRES) {
      union node* g = expand_glob(nptr, n->narg.flag);

      if(g) {
        n = g;
        nptr = &n;
        ret++;
      }

      stralloc_nul(&n->narg.stra);
    } else if((n->narg.flag & X_LITERAL) && !(n->narg.flag & X_UNESCAPED)) {
      expand_unescape(&n->narg.stra, parse_isesc);
      n->narg.flag &= ~X_GLOB;
    } else {
      /* expand_unescape() nul-terminates as a side effect; skipping it
         here must not also skip that, or ->stra.s stops being a valid
         C string for anything that reads it as one. */
      stralloc_nul(&n->narg.stra);
    }

    if(arg->next) {
      n->next = tree_newnode(N_ARG);
      n = n->next;
      stralloc_init(&n->narg.stra);
      stralloc_nul(&n->narg.stra);
      ret++;
    }
  }

  /* expand_arg() may hand back a chain (field splitting, "$@"); only its
     last node was terminated above, expand_argv() needs every one
     (nptr itself now points at the local "n", so walk from "head") */
  for(n = *head; n; n = n->next)
    if(n->narg.stra.s)
      stralloc_nul(&n->narg.stra);

  if(copied && owned)
    tree_free(owned);

  return ret;
}
