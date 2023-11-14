#include "../parse.h"
#include "../prompt.h"
#include "../source.h"
#include "../tree.h"

unsigned int parse_lineno;

/* parse simple tokens consisting of 1 or 2 chars
 * ----------------------------------------------------------------------- */
enum tok_flag
parse_simpletok(struct parser* p) {
  char c;
  enum tok_flag tok;
  int advance;

  /* get a char but do not remove it from the buffer yet */
  for(;;) {
    if(source_peek(&c) <= 0)
      return T_EOF;

    /* skip all whitespace */
    while(parse_isspace(c)) {
      /* break on a newline if we aren't skipping them */
      if(!(p->flags & P_SKIPNL) && c == '\n')
        break;

      /* if the char was skipped, get next one */
      if(source_next(&c) <= 0)
        return T_EOF;
    }

    advance = 1;

    p->tokstart = source->position;
    /* now we have a non-space char */
    switch(c) {
      /* skip comments */
      case '#':
        for(;;) {
          if(c == '\n') {
            if(source_peeknc(1) != '#')
              break;
          }

          if(p->flags & P_COMMENT)
            stralloc_catc(&p->sa, c);

          if(source_next(&c) <= 0)
            return T_EOF;
        }

        if(p->flags & P_COMMENT)
          return -2;

        /* after getting chars fall into newline case */
        goto newline;
      /* check for escaped newline (line continuation) */
      case '\\':
        if(source_peekn(&c, 1) <= 0)
          return T_EOF;

        /* CRAP CODE to support win, mac, unix line termination */
        if(c == '\r') {

          if(source_next(&c) <= 0)
            return T_EOF;

          if(c == '\n')
            parse_skip(p);
          source_skip();
#if !defined(SHFORMAT) && !defined(SHPARSE2AST)
          prompt_show();
#endif
          continue;
        }
        if(c == '\n') {
          parse_skip(p);
          parse_skip(p);

          continue;
        }
        /* END OF CRAP CODE to be fixed */

        return -1;
      /* might be a mac or a windows newline */
      case '\r':
        if(source_next(&c) <= 0)
          return T_EOF;

        if(c == '\n')
          parse_skip(p);
      /* encountered a new line */
      case '\n':
      newline:
        if(p->flags & P_SKIPNL)
          continue;
        tok = T_NL;
        break;
      /* check for a pipe char, and then check for || */
      case '|': tok = T_PIPE; goto checkdouble;
      /* check for a background char, and then check for && */
      case '&': tok = T_BGND; goto checkdouble;
      /* check for a semicolon, and then check for ;; */
      case ';':
        tok = T_SEMI;

        /* check if the next char is the same */
      checkdouble : {
        char c2;

        /* advance buffer position now, but not later */
        advance = 0;

        /* peek a char and look it it's the same */
        if(source_next(&c2) > 0 && c == c2) {
          /* advance buffer position later, because the char
             we peeked was valid */
          advance = 1;

          /* do not change order of the ;/;;, &/&&, |/|| tokens,
             they must be subsequent to each other for the next
             line to work */
          tok <<= 1;
        }

        break;
      }

        /* begin or end a subshell */
      case '(': tok = T_LP; break;
      case ')': tok = T_RP; break;
      /* handle backquote as (ending) token only when
         we're in a backquoted cmd list */
      case '`':
        if(p->flags & P_BQUOTE) {
          tok = T_BQ;
          break;
        }

      default: return -1;
    }

    break;
  }

  if(advance)
    source_skip();

  return tok;
}
