#define DEBUG_NOCOLOR 1
#include "../debug.h"

#if defined(DEBUG_OUTPUT) || defined(SHPARSE2AST)
#include "../fd.h"
#include "../redir.h"
#include "../../lib/str.h"
#include <string.h>

/* output redirection flag string
 * ----------------------------------------------------------------------- */
void
debug_redir(const char* msg, int flags, int depth) {
  /* static char flagstr[128];

   switch(flags & (R_IN | R_OUT)) {
     case R_IN | R_OUT: str_copy(flagstr, "R_IN|R_OUT"); break;
     case R_IN: str_copy(flagstr, "R_IN"); break;
     case R_OUT: str_copy(flagstr, "R_OUT"); break;
   }

   if(flags & R_OPEN)
     strcat(flagstr, COLOR_CYAN "|" COLOR_MAGENTA "R_OPEN");

if(flags & R_DUP)
     strcat(flagstr, COLOR_CYAN "|" COLOR_MAGENTA "R_DUP");

if(flags & R_HERE)
     strcat(flagstr, COLOR_CYAN "|" COLOR_MAGENTA "R_HERE");

if(flags & R_STRIP)
     strcat(flagstr, COLOR_CYAN "|" COLOR_MAGENTA "R_STRIP");

if(flags & R_APPEND)
     strcat(flagstr, COLOR_CYAN "|" COLOR_MAGENTA "R_APPEND");

if(flags & R_CLOBBER)
     strcat(flagstr, COLOR_CYAN "|" COLOR_MAGENTA "R_CLOBBER");*/
  /*
    if(msg) {
      debug_s(COLOR_YELLOW);
      debug_s(msg);
      debug_s(COLOR_CYAN DEBUG_EQU);
    }*/
  debug_ulong(msg, flags, depth);
  /*debug_s(COLOR_MAGENTA);
  debug_s(flagstr);
  debug_s(COLOR_NONE);
*/
  // debug_space(depth, 0);
  debug_fl();
}
#endif /* DEBUG_OUTPUT */
