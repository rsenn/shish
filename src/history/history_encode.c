#include "../history.h"

/* encode a (possibly multi-line) command into a single history line.
 *
 * the on-disk format is strictly one entry per line, so the line
 * iterators never have to parse shell quoting to find where an entry
 * ends -- they just look for '\n'. a literal backslash or newline in
 * the command is therefore escaped here, and unescaped again by
 * history_decode.
 * ----------------------------------------------------------------------- */
void
history_encode(const char* s, size_t len, stralloc* out) {
  size_t i;

  stralloc_zero(out);

  for(i = 0; i < len; i++) {
    switch(s[i]) {
      case '\\': stralloc_cats(out, "\\\\"); break;
      case '\n': stralloc_cats(out, "\\n"); break;
      case '\r': stralloc_cats(out, "\\r"); break;
      default: stralloc_catb(out, &s[i], 1); break;
    }
  }
}
