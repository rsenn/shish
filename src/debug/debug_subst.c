#include "debug.h"

#ifdef DEBUG
#include "expand.h"
#include "fd.h"
#include "str.h"
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
  static char flagstr[128];
  unsigned long n = 0;

  flagstr[n] = '\0';

  if(flags & S_SPECIAL) {
    n += str_copy(&flagstr[n], debug_subst_special[(flags >> 3) & 0x1f]);
    flagstr[n++] = '|';
  }

  n += str_copy(&flagstr[n], debug_subst_var[(flags >> 8) & 0x0f]);

  flagstr[n++] = '|';
  n += str_copy(&flagstr[n], debug_subst_tables[flags & S_TABLE]);

  if(flags & S_BQUOTE)
    n += str_copy(&flagstr[n], "|S_BQUOTE");
  if(flags & S_STRLEN)
    n += str_copy(&flagstr[n], "|S_STRLEN");
  if(flags & S_NULL)
    n += str_copy(&flagstr[n], "|S_NULL");
  if(flags & S_NOSPLIT)
    n += str_copy(&flagstr[n], "|S_NOSPLIT");
  if(flags & S_GLOB)
    n += str_copy(&flagstr[n], "|S_GLOB");
  if(flags & S_ESCAPED)
    n += str_copy(&flagstr[n], "|S_ESCAPED");

  buffer_puts(fd_err->w, COLOR_YELLOW);
  buffer_puts(fd_err->w, msg);
  buffer_puts(fd_err->w, COLOR_CYAN DEBUG_EQU COLOR_GREEN);
  buffer_puts(fd_err->w, flagstr);
  buffer_puts(fd_err->w, COLOR_NONE);
}
#endif /* DEBUG */
