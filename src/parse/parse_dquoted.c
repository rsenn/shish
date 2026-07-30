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
         general, but inside a here-document it must instead be
         treated as an implicit line terminator for whatever's
         accumulated so far -- exactly like a real '\n' would be,
         complete with the synthetic newline appended below so
         parse_here.c's delimiter-length comparison still lines up.
         Without this, a here-doc whose closing delimiter is the very
         last thing in the source (no newline after it -- routine for
         a `-c` script string, which unlike a file has no implicit
         trailing newline of its own) never matched the delimiter at
         all: parsing errored out here instead, and redir_source.c's
         error path left the redirection's word as the original,
         unexpanded delimiter node -- so the here-doc's "content"
         silently became the literal delimiter text instead of the
         real body. */
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
         immediately followed by a newline wherever they see one --
         that swallow is unconditional, applied before any
         quoting-mode code ever gets to inspect the raw bytes, so by
         the time source_next() returns here, 'nextc' is already
         whatever came *after* the continuation, not the newline
         itself. A bumped source line number is the only remaining
         trace that a continuation was actually swallowed just now
         (as opposed to 'nextc' genuinely being some other,
         non-continuation character). POSIX requires the exact same
         backslash-newline removal inside double quotes as outside
         them, so this round must produce nothing at all -- not even
         the literal backslash the code below would otherwise keep,
         since nothing this branch has seen so far actually matched a
         recognized double-quote escape target ($, `, ", \). Without
         this, "...\<newline>..." inside a double-quoted string kept
         the backslash as a literal character while still dropping
         the newline, corrupting the string (confirmed via autoconf's
         generated `configure`, which relies on exactly this
         construct and silently mis-evaluated a shell-compatibility
         probe as a result). */
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
