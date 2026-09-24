#include "sed_internal.h"
#include "../../lib/stralloc.h"

/* sed_text_parse: the argument to a/i/c. Two forms, both accepted:
 * POSIX's "a\" then one or more lines, each internal newline written
 * as "\<newline>" in the source; and GNU's one-line "a text" (text
 * starts right after the command on the same line; a trailing "\" at
 * end of line still continues it, same as the POSIX form). Only the
 * blanks between the command letter and the text itself (the syntax
 * separator, not text content) are skipped -- POSIX's own rule for
 * the text ("each embedded newline preceded by a backslash; any other
 * backslash is removed and the following character taken literally")
 * says nothing about stripping a text line's own leading blanks, so
 * indentation the script author wrote on purpose survives.
 * ----------------------------------------------------------------------- */
int
sed_text_parse(const char** pp, const char* end, struct sed_text* t) {
  const char* p = *pp;
  stralloc out;
  int rc = SED_OK;

  stralloc_init(&out);

  while(p < end && (*p == ' ' || *p == '\t'))
    p++;

  if(p < end && *p == '\\' && p + 1 < end && p[1] == '\n')
    p += 2;

  for(;;) {
    while(p < end && *p != '\n') {
      if(*p == '\\' && p + 1 < end) {
        if(p[1] == '\n') {
          if(!stralloc_catc(&out, '\n')) {
            rc = SED_ENOMEM;
            goto done;
          }

          p += 2;
          goto next_line;
        }

        if(!stralloc_catc(&out, (unsigned char)p[1])) {
          rc = SED_ENOMEM;
          goto done;
        }

        p += 2;
      } else {
        if(!stralloc_catc(&out, (unsigned char)*p)) {
          rc = SED_ENOMEM;
          goto done;
        }

        p++;
      }
    }

    if(p < end)
      p++;

    break;

  next_line:;
  }

  t->s = out.s;
  t->len = out.len;
  *pp = p;
  return SED_OK;

done:
  stralloc_free(&out);
  return rc;
}
