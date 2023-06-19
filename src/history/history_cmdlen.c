#include "../history.h"
#include "../parse.h"

unsigned long
history_cmdlen(const char* v) {
  const char* b = v;
  int quoted = 0;
  int mapping = 0;
  const char* mapend;
  const char* mapstart = (const char*)history_buffer.x;

  mapend = history_buffer.x + history_buffer.n;

  if(b >= mapstart && b <= mapend) {
    mapping = 1;

    /* skip leading newlines */
    while(b < mapend && parse_isspace(*b))
      b++;

    /* do not go beyond the map */
    if(b == mapend)
      return 0;
  } else {
    while(parse_isspace(*b))
      b++;
  }

  for(; mapping ? b < mapend : *b; b++) {
    /* to leave single quotation mode there must be a
       single quote which can't be escaped */
    if(quoted == 1) {
      if(*b == '\'')
        quoted = 0;
    }

    /* escape stuff, except for single-quotation */
    else if(*b == '\\')
      b++;

    /* double quotation */
    else if(quoted) {
      if(*b == '\"')
        quoted = 0;
    }

    /* on a single quote change the mode */
    else if(*b == '\'')
      quoted++;

    /* on a double quote too */
    else if(*b == '"')
      quoted = 2;

    /* have we really skipped something since the last entry?
       continue if not! */
    else if(b == v)
      continue;

    /* break on a line termination */
    else if((*b == '\n' || *b == '\r'))
      break;
  }

  /* calc len */
  return b - v;
}
