#include "../parse.h"
#include "../sh.h"
#include "../source.h"
#include "../tree.h"
#include "../../lib/alloc.h"

/* $'...' (ANSI-C quoting): decode the same backslash escapes as
 * `echo -e` (sh_unescape()) and splice the result in as literal,
 * single-quoted text. parse_subst() only calls this outside of
 * double quotes -- "$'...'" is plain literal text there, not this
 * syntax (bash and ksh agree on this).
 *
 * \' is handled here rather than in sh_unescape(), since it must
 * close the quote everywhere else but here, and `echo -e` has no
 * such rule to share.
 * ----------------------------------------------------------------------- */
int
parse_ansiquoted(struct parser* p) {
  stralloc raw;
  char* dec;
  size_t declen, i;
  char c;

  stralloc_init(&raw);

  for(;;) {
    if(source_get(&c) <= 0) {
      stralloc_free(&raw);
      return -1;
    }

    if(c == '\\') {
      char nextc;

      if(source_get(&nextc) <= 0) {
        stralloc_free(&raw);
        return -1;
      }

      if(nextc == '\'') {
        stralloc_catc(&raw, '\'');
      } else {
        stralloc_catc(&raw, c);
        stralloc_catc(&raw, nextc);
      }

      continue;
    }

    if(c == '\'')
      break;

    stralloc_catc(&raw, c);
  }

  stralloc_nul(&raw);
  dec = alloc(raw.len ? raw.len : 1);
  declen = sh_unescape(raw.s, raw.len, dec);

  parse_string(p, 0);
  p->quot = Q_SQUOTED;

  for(i = 0; i < declen; i++) {
    if(parse_isesc(dec[i]))
      stralloc_catc(&p->sa, '\\');

    stralloc_catc(&p->sa, dec[i]);
  }

  parse_string(p, 0);
  p->quot = Q_UNQUOTED;

  alloc_free(dec);
  stralloc_free(&raw);
  return 0;
}
