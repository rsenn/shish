#include "str.h"
#include "tree.h"
#include "parse.h"

static char parse_namebuf[128];

/* return token name 
 * ----------------------------------------------------------------------- */
const char *parse_tokname(enum tok_flag tok, int multi)
{
  unsigned int i;
  unsigned int di = 0;

  for(i = 0; i < sizeof(tok) * 8; i++)
  {
    if((tok >> i) & 0x01)
    {
      if(di) parse_namebuf[di++] = ',';
      di += str_copy(&parse_namebuf[di], parse_tokens[i].name);
      if(!multi) break;
    }
  }
  return parse_namebuf;
}

