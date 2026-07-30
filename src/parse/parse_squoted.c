#include "../parse.h"
#include "../source.h"
#include "../tree.h"

int
parse_squoted(struct parser* p) {
  char c;

  if(p->tok == T_NAME)
    p->tok = T_WORD;

  p->quot = Q_SQUOTED;

  /* source_get()/source_skip() read through source_peekn(), which
     otherwise unconditionally treats a backslash-newline as a line
     continuation and silently drops both bytes -- correct outside
     quotes and inside double quotes, but POSIX requires single quotes
     (and, the same way, a heredoc with a quoted delimiter, which
     reuses this same function -- P_HERE below) to preserve every
     character completely literally. Cleared on every exit path below,
     not just the normal one: source_squoted is a single shared flag,
     not scoped to this call, so leaving it set on an early return
     would wrongly suppress continuation handling for everything read
     afterward. squoted-backslash-newline-swallowed, fixes/108. */
  source_squoted = 1;

  for(;;) {
    if(source_get(&c) <= 0) {
      source_squoted = 0;

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
      source_squoted = 0;
      parse_string(p, 0);
      p->quot = Q_UNQUOTED;
      break;
    }

    if(parse_isesc(c) && !(p->flags & P_HERE))
      stralloc_catc(&p->sa, '\\');

    stralloc_catc(&p->sa, c);

    if((p->flags & P_HERE) && c == '\n') {
      source_squoted = 0;
      return 0;
    }
  }

  parse_string(p, 0);
  return 0;
}
