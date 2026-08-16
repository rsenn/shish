#include "../parse.h"
#include "../source.h"
#include "../tree.h"

int
parse_dquoted(struct parser* p) {
  int flags = p->flags;
  char c;

  if(p->tok == T_NAME)
    p->tok = T_WORD;

  /* set the quotation mode */
  p->quot = Q_DQUOTED;

  for(;;) {
    /* peek the next char */
    if(source_peek(&c) <= 0) {
      /* end of input with no trailing newline is a hard error in
         general, but inside a here-document it's an implicit line
         terminator for whatever's accumulated so far -- append a
         synthetic newline so parse_here.c's delimiter-length
         comparison still lines up. */
      if(flags & P_HERE) {
        stralloc_catc(&p->sa, '\n');
        break;
      }

      return -1;
    }

    /* only ", $ and ` must be escaped */
    if(c == '\\') {
      char nextc;
      unsigned int lineno = source->position.line;

      if(source_next(&nextc) <= 0)
        return -1;

      /* source_skip()/source_peekn() silently remove a backslash
         immediately followed by a newline wherever they see one, so
         by the time source_next() returns here, 'nextc' is already
         whatever came after the continuation. A bumped source line
         number is the only remaining trace that a continuation was
         swallowed. POSIX requires the same removal inside double
         quotes as outside, so this round must produce nothing at all,
         not even the literal backslash. */
      if(source->position.line != lineno)
        continue;

      if(parse_isdesc(nextc) || nextc == '\\') {
        c = nextc;
        source_skip();
      }
    } else if(c == '`') {
      if((flags & P_BQUOTE))
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
    else if(!(flags & P_HERE) && c == '"') {
      parse_skip(p);
      parse_string(p, 0);
      p->quot = Q_UNQUOTED;
      break;
    } else {
      parse_skip(p);
    }

    if(parse_isesc(c) && !(flags & P_HERE))
      stralloc_catc(&p->sa, '\\');

    stralloc_catc(&p->sa, c);

    /* return on a newline for the here-doc delimiter check */
    if((flags & P_HERE) && c == '\n')
      break;
  }

  if(!(flags & P_HERE))
    parse_string(p, flags);

  return 0;
}
