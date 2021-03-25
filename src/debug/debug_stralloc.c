#define DEBUG_NOCOLOR 1
#include "../debug.h"
#include "../expand.h"

#ifdef DEBUG_OUTPUT
#include "../fd.h"

/* output an allocated string
 * ----------------------------------------------------------------------- */
void
debug_stralloc(const char* msg, stralloc* s, int depth, char quote) {
  if(msg)
    debug_field(msg, depth);

  debug_s(quote ? COLOR_CYAN : COLOR_YELLOW);
  if(quote)
    debug_c(quote);

  if(s->len && s->s) {
    const char* x = s->s;
    const char* end = s->s + s->len;

    while(x < end) {
      if(*x == '\n')
        debug_s("\\n");
      else if(*x == '\r')
        debug_s("\\r");
      else
        debug_c(*x);
      x++;
    }
  }
  if(quote)
    debug_c(quote);

  debug_s(COLOR_NONE);
  debug_fl();
}
#endif /* DEBUG_OUTPUT */
