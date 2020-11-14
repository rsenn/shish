#include "../../lib/uint64.h"
#include "../../lib/byte.h"
#include "../../lib/shell.h"
#include "../../lib/str.h"
#include "../../lib/stralloc.h"
#include "../../lib/buffer.h"
#include "../fd.h"
#include "../expand.h"
#include "../fdstack.h"
#include "../parse.h"
#include "../source.h"
#include "../tree.h"
#include "../debug.h"

/* parse and expruate arguments
 * ----------------------------------------------------------------------- */
int
builtin_expr(int argc, char* argv[]) {
  struct fd fd;
  struct source src;
  struct parser p;
  union node* expr;
  int ret = 0;
  int64 result = 0;
  stralloc sa;
  stralloc_init(&sa);

  if(argc == 1) {
    ret = 1;
  } else if(!str_diff(argv[1], "length")) {
    result = argv[2] ? str_len(argv[2]) : 0;

  } else if(!str_diff(argv[1], "index")) {
    const char* haystack = argc >= 3 ? argv[2] : "";
    const char* needle = argc >= 4 ? argv[3] : "";
    size_t i, n = str_len(haystack) - str_len(needle);

    for(i = 0; i < n; i++) {
      if(!byte_diff(&haystack[i], str_len(needle), needle)) {
        result = i + 1;
        break;
      }
    }

  } else {
    int i;

    /* concatenate all arguments following the "expr", separated by a
       whitespace and terminated by a newline */
    for(i = 1; i < argc; i++) {
      stralloc_cats(&sa, argv[i]);
      stralloc_catc(&sa, i + 1 < argc ? ' ' : '\0');
    }

    /* create a new i/o context and initialize a parser */
    fd_push(&fd, STDSRC_FILENO, FD_READ);
    fd_string(&fd, sa.s, sa.len);
    source_push(&src);

    parse_init(&p, P_ARITH | P_NOREDIR);

    /* parse the string as a compound list */
    if((expr = parse_arith_expr(&p))) {

#ifdef DEBUG_OUTPUT
      debug_list(expr, 0);
      buffer_putnlflush(fd_err->w);
#endif /* DEBUG_OUTPUT */

      if(expand_arith_expr(expr, &result)) {
        ret = 1;
      }
      tree_free(expr);
    }

    source_pop();
    fd_pop(&fd);
  }

  if(ret == 0) {
    buffer_putlonglong(fd_out->w, result);
    buffer_putnlflush(fd_out->w);
  }

  return ret;
}
