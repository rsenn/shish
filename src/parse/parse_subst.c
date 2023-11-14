#include "../parse.h"
#include "../source.h"
#include "../tree.h"

int
parse_subst(struct parser* p) {
  char c[3];
  int ret = 0;

  if(source_peek(&c[0]) <= 0)
    return -1;

  if(c[0] == '$')
    source_next(&c[0]);

  if(c[0] == '(') {
    if(source_peekn(&c[1], 1) <= 0)
      return -1;

    if(c[1] == '(') {
      source_skip();
      return parse_arith(p);
    }

    // source_skip();

    return parse_bquoted(p);
  } else if(parse_isparam(c[0]) || c[0] == '{') {
    return parse_param(p);
  }

  stralloc_catc(&p->sa, '$');
  return ret;
}
