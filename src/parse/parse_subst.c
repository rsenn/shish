#include "tree.h"
#include "parse.h"
#include "source.h"

int parse_subst(struct parser *p)
{
  unsigned char c;
  
  if(source_next(&c) <= 0)
    return -1;
  
  if(c == '(')
  {
    return parse_bquoted(p);
  }
  else if(parse_isparam(c) || c == '{')
  {
    return parse_param(p);
  }
  
  stralloc_catc(&p->sa, '$');
  return 0;
}
