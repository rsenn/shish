#include "../expand.h"
#include "../parse.h"
#include "../redir.h"
#include "../../lib/scan.h"
#include "../source.h"
#include "../tree.h"

int
parse_unquoted(struct parser* p) {
  int flags = 0;
  char c;

  /* set the quotation mode */
  p->quot = Q_UNQUOTED;

  for(;;) {
    /* get the next char */
    if(source_peek(&c) <= 0)
      return -1;

    /* everything can be escaped */
    if(c == '\\') {
      char nextc;

      p->tok = T_WORD;

      if(source_next(&nextc) <= 0)
        return -1;

      if(parse_isesc(nextc))
        stralloc_catc(&p->sa, '\\');

      /* remember when escaped for here-delimiter */
      flags |= S_ESCAPED;

      c = nextc;
    }

    /* when spotting double-quotes enter double-quotation mode */
    else if(c == '"') {
      parse_string(p, 0);

      parse_skip(p);
      p->quot = Q_DQUOTED;

      parse_string(p, 0);
      break;
    }

    /* when spotting single-quote enter single-quotation mode */
    else if(c == '\'') {
      parse_string(p, 0);

      parse_skip(p);
      p->quot = Q_SQUOTED;

      parse_string(p, 0);
      break;
    }

    /* when spotting backquote enter command substitution mode */
    else if(c == '`') {

      /* if we're already parsing backquoted stuff then we should
         terminate the current subst instead of creating a new one
         inside. */
      if(p->flags & P_BQUOTE) {
        /*  parse_gettok(p, 0);
         p->pushback++; */
        return 1;
      }

      parse_string(p, 0);

      if(parse_bquoted(p))
        break;

      continue;
    }
    /* when spotting $ enter parameter substitution mode */
    else if(c == '$') {
      parse_string(p, 0);

      if(parse_subst(p))
        break;

      continue;
    }
    /* check for redirections */
    else if((p->flags & P_NOREDIR) == 0 && (c == '<' || c == '>')) {
      int fd = (c == '<' ? 0 : 1);

      if(p->sa.len == 0 || scan_uint(p->sa.s, (unsigned int*)&fd) == p->sa.len)
        return redir_parse(p, (c == '<' ? R_IN : R_OUT), fd);
    }

    /* on a substition word in ${name:word} we parse until a right brace occurs
     */
    else if(p->flags & P_SUBSTW) {
      if(c == '}') {
        parse_skip(p);
        parse_string(p, flags);
        return 1;
      }
    }

    /* ...when spotted a delimiter (space, or first char of an operator token)
     */
    else if(parse_isctrl(c) || parse_isspace(c)) {
      /* if we're looking for keywords, there is no word tree and
         there is a string in the parser we check for keyworsd */
      if((p->flags & P_NOKEYWD) || p->tree || p->sa.s == NULL || !parse_keyword(p))
        parse_string(p, flags);

      return 1;
    }
    /* if it is a character subject to globbing then set S_GLOB flag */
    else if(parse_isesc(c)) {
      flags |= S_GLOB;
    }

    if(p->tok == T_NAME && p->sa.len && c == '=')
      p->tok = T_ASSIGN;

    if(p->tok == T_NAME && !parse_isfname(c, p->sa.len))
      p->tok = T_WORD;

    stralloc_catc(&p->sa, c);
    parse_skip(p);
  }

  return 0;
}
