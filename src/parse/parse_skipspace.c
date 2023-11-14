#include "../parse.h"
#include "../source.h"
#include "../tree.h"

/* skip any unquoted whitespace preceeding a word
 * ----------------------------------------------------------------------- */
enum tok_flag
parse_skipspace(struct parser* p) {
  char c;

  /* skip whitespace */
  for(;;) {
    if(source_peek(&c) <= 0)
      return T_EOF;

    if(c == '\n') {
      //parse_skip(p);

      /* in a here-doc skip the newline after the delimiter */
      if(p->flags & P_HERE)
        break;

      /* skip leading newlines if requested so */
      if(!(p->flags & P_SKIPNL))
        return T_NL;

      continue;
    } else if(!parse_isspace(c))
      break;

    parse_skip(p);
  }

  return -1;
}
