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
  int ret = 0;
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

  /* parse the string as a compound list */
  if((cmds = parse_compound_list(&p, 0))) {
    eval_push(&e, sh->opts.xtrace ? E_PRINT : 0);
    eval_tree(&e, cmds, E_ROOT | E_LIST);
    ret = eval_pop(&e);
    tree_free(cmds);
  }

  /* a syntax error in the argument string is a shell language syntax
     error, so it has to be reported and fail. "eval" is a special
     builtin, so returning nonzero is also what ends a non-interactive
     shell (exec_command.c). A NULL tree alone does not mean an error
     -- "eval ''" parses fine and does nothing -- the stopping token
     is what tells them apart. */
  if(!(p.tok & (T_EOF | T_NL | T_SEMI | T_BGND))) {
    parse_error(&p, 0);
    ret = 1;
  }

  source_popfd(&fd);

  return ret;
}
