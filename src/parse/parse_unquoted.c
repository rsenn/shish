#include "../expand.h"
#include "../parse.h"
#include "../redir.h"
#include "../../lib/scan.h"
#include "../source.h"
#include "../tree.h"

int
parse_unquoted(struct parser* p) {
  int flags = 0;
  int in_bracket = 0;
  char c;

  /* set the quotation mode */
  p->quot = Q_UNQUOTED;

  for(;;) {
    /* get the next char */
    if(source_peek(&c) <= 0) {
      /* true end-of-input, not just "no delimiter yet": mirror the
         delimiter branch below -- try a keyword match first (don't
         flush if this turns out to be one), and only then flush with
         the locally-tracked flags (S_GLOB in particular), which live
         only in this stack frame and would otherwise be lost for a
         word ending exactly at EOF. */
      if((p->flags & P_NOKEYWD) || p->tree || p->sa.s == NULL || !parse_keyword(p))
        parse_string(p, flags);
      return -1;
    }

    /* everything can be escaped */
    if(c == '\\') {
      char nextc;

      /* don't downgrade T_ASSIGN: a backslash in the assignment value
         (e.g. `x=\'`) is part of the value, not the variable name. */
      if(p->tok != T_ASSIGN)
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

      /* scan_uint() reads a plain C string and has no idea p->sa is
         only supposed to hold p->sa.len bytes; p->sa is reused
         scratch space that never clears its old bytes on reset, so it
         must be nul-terminated here first or a short digit prefix can
         read through into a stale trailing digit. */
      stralloc_nul(&p->sa);

      /* an out-of-range [n] prefix (e.g. "99999<&1") must not reach
         fd_push()/fdtable_link(), which index fdtable[n] unchecked --
         fall through to ordinary word parsing instead. */
      if(p->sa.len == 0 ||
         (scan_uint(p->sa.s, (unsigned int*)&fd) == p->sa.len && fd >= 0 && fd < FD_MAX))
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
    /* '[' only makes a word worth handing to glob(3) if it's later
       matched by a ']' in the same word -- an unpaired '[' is never a
       valid bracket expression, so it stays literal and skips the
       real glob(3) directory scan a stray S_GLOB flag would trigger. */
    else if(c == '[') {
      in_bracket = 1;
    } else if(c == ']') {
      if(in_bracket) {
        flags |= S_GLOB;
        in_bracket = 0;
      }
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
