#include "../parse.h"
#include "../source.h"
#include "../tree.h"

int
parse_dquoted(struct parser* p) {
  int flags = 0;
  char c;

  if(p->tok == T_NAME)
    p->tok = T_WORD;

  /* set the quotation mode */
  p->quot = Q_DQUOTED;

  for(;;) {
    /* peek the next char */
    if(source_peek(&c) <= 0)
      return -1;

    /* only ", $ and ` must be escaped */
    if(c == '\\') {
      char nextc;

      if(source_next(&nextc) <= 0)
        return -1;

      if(parse_isdesc(nextc)) {
        parse_skip(p);
        c = nextc;
      }
    } else if(c == '`') {
      if((p->flags & P_BQUOTE))
        break;
      parse_string(p, 0);
      if(parse_bquoted(p))
        break;
      continue;
    } else if(c == '$') {
      parse_string(p, 0);
      if(parse_subst(p))
        break;
      continue;
    }
    /* when spotted a closing quote,
       skip it and unset quotation mode */
    else if(!(p->flags & P_HERE) && c == '"') {
      parse_skip(p);
      parse_string(p, 0);
      p->quot = Q_UNQUOTED;
      break;
    } else {
      parse_skip(p);
    }

    if(parse_isesc(c) && !(p->flags & P_HERE))
      stralloc_catc(&p->sa, '\\');

    stralloc_catc(&p->sa, c);

    /* return on a newline for the here-doc delimiter check */
    if((p->flags & P_HERE) && c == '\n')
      break;
  }

  parse_string(p, flags);
  return 0;
}
