#include "../expand.h"
#include "../tree.h"
#include "../debug.h"
#include "../fd.h"
#include "../parse.h"
#include "../sh.h"

/* fields of all expansions, one command's worth at a time; the caller rewinds it */
arena expand_arena = {NULL, NULL, NULL, NULL, &arena_heap, 8192};

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
       (sub->nargparam.flag & S_TABLE) == S_DQUOTED && !(sub->nargparam.flag & S_STRLEN) && !sub->nargparam.word) {
      at = 1;
      continue;
    }

    return 0;
  }

  return at;
}

/* expand all arguments of an argument list into wl, one word after the other
 * returns the number of fields the words added
 * ----------------------------------------------------------------------- */
int
expand_args(union node* args, wordlist* wl, int flags) {
  union node *arg, *owned;
  int ret = 0, copied;

  wl->noglob = sh->opts.noglob;

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
    if(expand_is_empty_at(arg))
      continue;

    if(copied)
      expand_tilde_word(arg);

    expand_arg(arg->narg.list, wl, flags);
    ret += wordlist_close(wl);
  }

  if(copied && owned)
    tree_free(owned);

  return ret;
}
