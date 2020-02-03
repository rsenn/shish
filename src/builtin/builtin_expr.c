#include "expand.h"
#include "fd.h"
#include "fdstack.h"
#include "parse.h"
#include "shell.h"
#include "source.h"
#include "stralloc.h"
#include "tree.h"

/* parse and expruate arguments
 * ----------------------------------------------------------------------- */
int
builtin_expr(int argc, char** argv) {
  struct fd fd;
  struct source src;
  struct parser p;
  stralloc sa;
  stralloc_init(&sa);
  union node* expr;
  int ret = 0;

  /* concatenate all arguments following the "expr", separated by a
     whitespace and terminated by a newline */
  shell_optind = 1;
  while(argv[shell_optind]) {
    stralloc_cats(&sa, argv[shell_optind]);
    stralloc_catc(&sa, (argv[++shell_optind] ? ' ' : '\n'));
  }

  /* create a new i/o context and initialize a parser */
  fd_push(&fd, STDSRC_FILENO, FD_READ);
  fd_string(&fd, sa.s, sa.len);
  source_push(&src);

  parse_init(&p, P_ARITH | P_NOREDIR);

  /* parse the string as a compound list */
  if((expr = parse_arith_expr(&p))) {
    int64 r;
    if(!expand_arith_expr(expr, &r)) {
      buffer_putlonglong(fd_out->w, r);
      buffer_putnlflush(fd_out->w);
    }
    tree_free(expr);
  }

  source_pop();
  fd_pop(&fd);

  return ret;
}
