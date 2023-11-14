#define DEBUG_NOCOLOR 1
#include "../debug.h"

#if defined(DEBUG_OUTPUT) || defined(SHPARSE2AST)
#include "../expand.h"
#include "../fd.h"
#include "../../lib/str.h"
#include "../tree.h"
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
    "S_DEFAULT",
    "S_ASGNDEF",
    "S_ERRNULL",
    "S_ALTERNAT",
    "S_RSSFX",
    "S_RLSFX",
    "S_RSPFX",
    "S_RLPFX",
};

const char* debug_subst_tables[] = {
    "S_UNQUOTED",
    "S_DQUOTED",
    "S_SQUOTED",
    "S_EXPR",
};

void
debug_subst(const char* msg, int flags) {
  static char flagstr[128];
  unsigned long n = 0;
  int subst;

  flagstr[n] = '\0';

  if(flags & S_SPECIAL) {
    n += str_copy(&flagstr[n], debug_subst_special[(flags >> 3) & 0x1f]);
  }

  subst = ((flags & S_VAR) >> 8) & 0x0f;

  if(subst) {
    if(n)
      n += str_copy(&flagstr[n], " | ");
    n += str_copy(&flagstr[n], debug_subst_var[subst]);
  }
  if(flags & S_TABLE) {
    if(n)
      n += str_copy(&flagstr[n], " | ");
    n += str_copy(&flagstr[n], debug_subst_tables[flags & S_TABLE]);
  }
  if(flags & S_BQUOTE) {
    n += str_copy(&flagstr[n], " | S_BQUOTE");
  }
  if(flags & S_STRLEN)
    n += str_copy(&flagstr[n], " | S_STRLEN");

  if(flags & S_NULL)
    n += str_copy(&flagstr[n], " | S_NULL");

  if(flags & S_NOSPLIT)
    n += str_copy(&flagstr[n], " | S_NOSPLIT");

  if(flags & S_GLOB)
    n += str_copy(&flagstr[n], " | S_GLOB");

  if(flags & S_ESCAPED)
    n += str_copy(&flagstr[n], " | S_ESCAPED");

  if(msg) {
    debug_s(COLOR_YELLOW);
    debug_s(msg);
    debug_s(COLOR_CYAN DEBUG_EQU COLOR_NONE);
  }

  debug_s(COLOR_MAGENTA);
  debug_s(flagstr);
  debug_s(COLOR_NONE);
  debug_fl();
}
#endif /* DEBUG_OUTPUT */
