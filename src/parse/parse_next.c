#include "../parse.h"
#include "../debug.h"
#include "../source.h"

/* ----------------------------------------------------------------------- */
int
parse_next(struct parser* p, char* c) {
#ifdef DEBUG_OUTPUT
  char ch;
  source_peek(&ch);

  if(ch == '\n') {
  debug_s("\x1b[1;33mparse_next");
  if(p->quot)
    debug_ulong("quot", p->quot, 0);

  debug_char(" ", ch, 0);
  debug_nl();
}
#endif

  if(c)
    return source_next(c);
  source_skip();
  return 0;
}
