#include "../parse.h"
#include "../tree.h"
#include "../expand.h"
#include "../fd.h"
#include "../../lib/str.h"
#include "../../lib/stralloc.h"

/* 3.9.5 - parse function definition
 * ----------------------------------------------------------------------- */
union node*
parse_function(struct parser* p) {
  int tok;
  union node* node;
  char c;

  stralloc name;
  stralloc_init(&name);
  tok = parse_gettok(p, 0);
  expand_tosa(p->tree, &name);
  node = tree_newnode(N_FUNCTION);
  stralloc_nul(&name);
  node->nfunc.name = name.s;

  p->flags |= P_SKIPNL;

  source_peek(&c);
  while(isspace(c) || c == '\n') source_next(&c);

  buffer_puts(fd_err->w, "in char: ");
  buffer_putc(fd_err->w, c);
  buffer_putnlflush(fd_err->w);

  node->nfunc.body = parse_grouping(&p);
  p->flags &= ~P_SKIPNL;

  return node;
}
