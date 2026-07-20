#include "../history.h"

/* inverse of history_encode: turn one stored history line back into the
 * command text it represents.
 * ----------------------------------------------------------------------- */
void
history_decode(const char* s, size_t len, stralloc* out) {
  size_t i;

  stralloc_zero(out);

  for(i = 0; i < len; i++) {
    if(s[i] == '\\' && i + 1 < len) {
      i++;

      switch(s[i]) {
        case 'n': stralloc_catc(out, '\n'); break;
        case 'r': stralloc_catc(out, '\r'); break;
        case '\\': stralloc_catc(out, '\\'); break;
        /* unknown escape (e.g. a hand-edited history file): keep both
           bytes verbatim rather than silently dropping the backslash */
        default:
          stralloc_catc(out, '\\');
          stralloc_catb(out, &s[i], 1);
          break;
      }
    } else
      stralloc_catb(out, &s[i], 1);
  }
}
