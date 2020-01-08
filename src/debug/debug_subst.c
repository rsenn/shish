#include "debug.h"

#ifdef DEBUG
#include "expand.h"
#include "fd.h"
#include "str.h"
#include "stralloc.h"
#include "tree.h"
#include <string.h>

/* output substitution flag string
 * ----------------------------------------------------------------------- */
const char* debug_subst_special[] = {
    NULL,
    "S_ARGC",
    "S_ARGV",
    "S_ARGVS",
    "S_EXITCODE",
    "S_FLAGS",
    "S_BGEXCODE",
    "S_ARG",
    "S_PID",
};

const char* debug_subst_var[] = {
    "S_DEFAULT", "S_ASGNDEF", "S_ERRNULL", "S_ALTERNAT", "S_RSSFX", "S_RLSFX", "S_RSPFX", "S_RLPFX"};

const char* debug_subst_tables[] = {"S_UNQUOTED", "S_DQUOTED", "S_SQUOTED", "S_EXPR"};

void
debug_subst(const char* msg, int flags, int depth) {
  stralloc sa;
  stralloc_init(&sa);

  if(flags & S_SPECIAL) {
    stralloc_cats(&sa,
                  debug_subst_special[(flags >> 3) % (sizeof(debug_subst_special) / sizeof(debug_subst_special[0]))]);
    stralloc_cats(&sa, " | ");
  }

  stralloc_cats(&sa, debug_subst_var[(flags >> 8) % (sizeof(debug_subst_var) / sizeof(debug_subst_var[0]))]);

  stralloc_cats(&sa, " | ");
  stralloc_cats(&sa, debug_subst_tables[flags & S_TABLE]);

  if(flags & S_BQUOTE)
    stralloc_cats(&sa, " | S_BQUOTE");
  if(flags & S_STRLEN)
    stralloc_cats(&sa, " | S_STRLEN");
  if(flags & S_NULL)
    stralloc_cats(&sa, " | S_NULL");
  if(flags & S_NOSPLIT)
    stralloc_cats(&sa, " | S_NOSPLIT");
  if(flags & S_GLOB)
    stralloc_cats(&sa, " | S_GLOB");
  if(flags & S_ESCAPED)
    stralloc_cats(&sa, " | S_ESCAPED");

  stralloc_nul(&sa);
  buffer_putm(fd_err->w, COLOR_YELLOW, msg, COLOR_CYAN, DEBUG_EQU, COLOR_GREEN, sa.s, COLOR_NONE, NULL);
  stralloc_free(&sa);
}
#endif /* DEBUG */
