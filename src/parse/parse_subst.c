#include "tree.h"
#include "parse.h"
#include "source.h"

int parse_subst(struct parser *p) {
  char c;
  
  if(source_next(&c) <= 0)
    return -1;
  
  if(c == '(')
  {
  	if(source_peek(&c) <= 0)
  		return -1;

  	if(c == '(') {
  		source_skip();
  		return parse_arith(p);
  	}

    return parse_bquoted(p);
  } else if(parse_isparam(c) || c == '{') {
    return parse_param(p);
  }
  
  stralloc_catc(&p->sa, '$');
  return 0;
}
