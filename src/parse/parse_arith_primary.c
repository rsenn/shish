#include "tree.h"
#include "parse.h"
#include "source.h"
#include "fd.h"
#include "scan.h"
#include "debug.h"

/* parse arithmetic primary
 * ----------------------------------------------------------------------- */
union node*
parse_arith_primary(struct parser *p) {
  union node *n;
  stralloc sa;
  stralloc_init(&sa);
  int isnum = 1;

  parse_skipspace(p);

  for(;;) {
    char c;

    if(source_peek(&c) < 1)
      break;

    if(!parse_isdigit(c) && !parse_isalnum(c) && c != '_')
      break;

    if(!parse_isdigit(c))
      isnum = 0;

    stralloc_catb(&sa, &c, 1);
    source_skip();
  }

  if(sa.len == 0) return NULL;
  stralloc_nul(&sa);

  n = tree_newnode(isnum ? N_ARITH_CONST : N_ARITH_IDENTIFIER);

  if(isnum) {
    long long num = 0;
    scan_longlong(sa.s, &n->narithnum.num);
    stralloc_free(&sa);
  } else {
    n->narithvar.var = sa.s;
    sa.s = NULL;
  }

#ifdef DEBUG
  debug_node(n, 2);
    buffer_putnlflush(fd_err->w);
#endif

  return n;
}
