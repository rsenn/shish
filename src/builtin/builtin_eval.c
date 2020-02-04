#include "../eval.h"
#include "../fd.h"
#include "../fdstack.h"
#include "../parse.h"
#include "../../lib/shell.h"
#include "../source.h"
#include "../../lib/stralloc.h"
#include "../tree.h"

/* parse and evaluate arguments
 * ----------------------------------------------------------------------- */
int
builtin_eval(int argc, char** argv) {
  struct fd fd;
  struct source src;
  struct parser p;
  struct eval e;
  union node* cmds;
  int ret = 0;
  stralloc sa;
  stralloc_init(&sa);

  /* concatenate all arguments following the "exec", separated by a
     whitespace and terminated by a newline */
  shell_optind = 1;
  while(argv[shell_optind]) {
    stralloc_cats(&sa, argv[shell_optind]);
    stralloc_catc(&sa, (argv[++shell_optind] ? ' ' : '\n'));
  }

  /* create a new i/o context and initialize a parser */
  fd_push(&fd, STDSRC_FILENO, D_READ);
  fd_string(&fd, sa.s, sa.len);
  source_push(&src);
  parse_init(&p, P_DEFAULT);

  /* parse the string as a compound list */
  if((cmds = parse_compound_list(&p))) {
    eval_push(&e, 0);
    eval_tree(&e, cmds, E_ROOT | E_LIST);
    ret = eval_pop(&e);
    tree_free(cmds);
  }

  source_pop();
  fd_pop(&fd);

  return ret;
}
