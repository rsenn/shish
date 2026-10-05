#include "builtin_config.h"

#if BUILTIN_EVAL

#include "../fd.h"
#include "../eval.h"
#include "../fdstack.h"
#include "../parse.h"
#include "../source.h"
#include "../tree.h"
#include "../sh.h"
#include "../../lib/shell.h"
#include "../../lib/stralloc.h"

/* parse and evaluate arguments
 * ----------------------------------------------------------------------- */
const char help_eval[] = "    Build and run a command from arguments.\n"
                         "\n"
                         "    args            joined with spaces, parsed and run as shell\n"
                         "                    input in the current shell environment\n";

int
builtin_eval(int argc, char* argv[]) {
  struct fd fd;
  struct source src;
  struct parser p;
  struct eval e;
  union node* cmds;
  int ret = 0, err = 0;
  size_t i;
  stralloc sa;
  stralloc_init(&sa);

  /* concatenate all arguments following the "exec", separated by a
     whitespace and terminated by a newline */
  i = 1;

  while(argv[i]) {
    stralloc_cats(&sa, argv[i]);
    stralloc_catc(&sa, (argv[++i] ? ' ' : '\n'));
  }

  /* create a new i/o context and initialize a parser */
  source_buffer(&src, &fd, sa.s, sa.len);
  parse_init(&p, P_DEFAULT);

  /* parse and run one command at a time, so that an alias defined by an
     earlier line is already in effect when the next one is parsed */
  eval_push(&e, sh->opts.xtrace ? E_PRINT : 0);

  while(!(parse_gettok(&p, P_DEFAULT) & T_EOF)) {
    p.pushback++;

    if((cmds = parse_list(&p))) {
      eval_tree(&e, cmds, E_ROOT | E_EVAL | E_LIST);
      tree_free(cmds);
    }

    /* a syntax error in the argument string is a shell language syntax
       error, so it has to be reported and fail. "eval" is a special
       builtin, so returning nonzero is also what ends a non-interactive
       shell (exec_command.c). */
    if(!(p.tok & (T_NL | T_SEMI | T_BGND))) {
      if(p.tok != T_EOF) {
        parse_error(&p, 0);
        err = 1;
      }
      break;
    }

    p.pushback = 0;
  }

  ret = eval_pop(&e);

  if(err)
    ret = 1;

  source_popfd(&fd);
  stralloc_free(&p.sa);
  stralloc_free(&sa);
  return ret;
}
#endif /* BUILTIN_EVAL */
