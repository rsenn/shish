#include "debug.h"

#ifdef DEBUG
#include "fd.h"
#include "redir.h"
#include "str.h"
#include <string.h>

/* output redirection flag string
 * ----------------------------------------------------------------------- */
void
debug_redir(const char* msg, int flags, int depth) {
  static char flagstr[128];

  switch(flags & (R_IN | R_OUT)) {
    case R_IN | R_OUT: str_copy(flagstr, "R_IN|R_OUT"); break;
    case R_IN: str_copy(flagstr, "R_IN"); break;
    case R_OUT: str_copy(flagstr, "R_OUT"); break;
  }

  if(flags & R_OPEN)
    strcat(flagstr, "|R_OPEN");
  if(flags & R_DUP)
    strcat(flagstr, "|R_DUP");
  if(flags & R_HERE)
    strcat(flagstr, "|R_HERE");
  if(flags & R_STRIP)
    strcat(flagstr, "|R_STRIP");
  if(flags & R_APPEND)
    strcat(flagstr, "|R_APPEND");
  if(flags & R_CLOBBER)
    strcat(flagstr, "|R_CLOBBER");

  buffer_putm(fd_err->w, COLOR_YELLOW, msg, COLOR_CYAN, DEBUG_EQU, COLOR_GREEN, flagstr, COLOR_NONE, NULL);

  debug_space(depth);
}
#endif /* DEBUG */
