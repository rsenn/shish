#include "../parse.h"
#include "../source.h"
#include "../tree.h"

int
parse_squoted(struct parser* p) {
  char c;

  if(p->tok == T_NAME)
    p->tok = T_WORD;

  p->quot = Q_SQUOTED;

  for(;;) {
    if(source_get(&c) <= 0) {
      /* same "end of input inside a here-doc must act like a
         trailing newline" fix as parse_dquoted.c -- see its comment
         for why. This is the nosubst (quoted-delimiter, e.g.
         "<<'EOF'") path, which needs it just as much. */
      if(p->flags & P_HERE) {
        stralloc_catc(&p->sa, '\n');
        return 0;
      }

      return -1;
    }

    if(!(p->flags & P_HERE) && c == '\'') {
      parse_string(p, 0);
      p->quot = Q_UNQUOTED;
      break;
    }

    if(parse_isesc(c) && !(p->flags & P_HERE))
      stralloc_catc(&p->sa, '\\');

    stralloc_catc(&p->sa, c);

    if((p->flags & P_HERE) && c == '\n')
      return 0;
  }

  parse_string(p, 0);
  return 0;
}
