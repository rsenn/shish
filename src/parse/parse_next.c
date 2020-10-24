#include "../parse.h"
#include "../debug.h"
#include "../source.h"

/* ----------------------------------------------------------------------- */
int
parse_next(struct parser* p, char* c) {
#ifdef DEBUG_OUTPUT_
  char ch;
  source_peek(&ch);
  debug_char("parse_next", ch, 0);
  debug_nl();
#endif

  if(c)
    return source_next(c);
  source_skip();
  return 0;
}
