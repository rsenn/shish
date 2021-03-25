#define DEBUG_NOCOLOR 1
#include "../debug.h"

#ifdef DEBUG_OUTPUT
#include "../fd.h"

/* output a nul-terminated string
 * ----------------------------------------------------------------------- */
void
debug_str(const char* msg, const char* s, int depth, char quote) {

  debug_s(quote ? COLOR_CYAN : COLOR_YELLOW);
  if(msg)
    debug_field(msg, depth);
  if(quote)
    debug_c(quote);

  if(s) {
    while(*s) {
      char c = *s;
      if(c == '\n')
        debug_s("\\n");
      else if(c == '\r')
        debug_s("\\r");
      else
        debug_c(c);
      s++;
    }
  }

  if(quote)
    debug_c(quote);
  if(quote)
    debug_s(COLOR_NONE);
  debug_fl();
}
#endif /* DEBUG_OUTPUT */
