#include "tree.h"
#include "parse.h"
#include "source.h"

/* skip any unquoted whitespace preceeding a word
 * ----------------------------------------------------------------------- */
int parse_skipspace(struct parser *p)
{
  char c;

  /* skip whitespace */
  for(;;)
  {
    if(source_peek(&c) <= 0)
      return T_EOF;

    if(c == '\n')
    {
      source_skip();
      
      /* in a here-doc skip the newline after the delimiter */
      if(p->flags & P_HERE)
        break;

      /* skip leading newlines if requested so */
      if(!(p->flags & P_SKIPNL))
        return T_NL;
      
      continue;
    }
    else if(!parse_isspace(c))
      break;

    source_skip();
  }

  return -1;
}

