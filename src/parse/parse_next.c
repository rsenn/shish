#include "../parse.h"

/* ----------------------------------------------------------------------- */
int
parse_next(struct parser* p, char* c) {
  char ch;
  source_peek(&ch);
  stralloc_catb(&p->sa, &ch, 1);
#ifdef DEBUG_OUTPUT
  debug_char("parse_next", ch);
#endif
  if(c)
    return source_next(c);
  source_skip();
  return 0;
}
