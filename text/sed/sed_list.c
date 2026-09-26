#include "sed_internal.h"

#define SED_LIST_WRAP 70 /* POSIX: fold width is unspecified; a common default */

/* sed_do_list: the 'l' command's visually-unambiguous rendering of
 * the pattern space (cat -v-style escapes, folded at SED_LIST_WRAP,
 * '$' at the true end). An embedded newline (from N) is shown as
 * "\n" like any other escape, not as a real line break -- only the
 * final, true end of the pattern space gets one.
 * ----------------------------------------------------------------------- */
void
sed_do_list(struct sed_state* st, const char* s, size_t n) {
  size_t i;
  size_t col = 0;
  char buf[4];

  for(i = 0; i < n; i++) {
    unsigned char c = (unsigned char)s[i];
    const char* piece = buf;
    size_t plen;

    switch(c) {
      case '\\':
        buf[0] = '\\';
        buf[1] = '\\';
        plen = 2;
        break;
      case '\a':
        buf[0] = '\\';
        buf[1] = 'a';
        plen = 2;
        break;
      case '\b':
        buf[0] = '\\';
        buf[1] = 'b';
        plen = 2;
        break;
      case '\f':
        buf[0] = '\\';
        buf[1] = 'f';
        plen = 2;
        break;
      case '\n':
        buf[0] = '\\';
        buf[1] = 'n';
        plen = 2;
        break;
      case '\r':
        buf[0] = '\\';
        buf[1] = 'r';
        plen = 2;
        break;
      case '\t':
        buf[0] = '\\';
        buf[1] = 't';
        plen = 2;
        break;
      case '\v':
        buf[0] = '\\';
        buf[1] = 'v';
        plen = 2;
        break;

      default:
        if(c < 0x20 || c >= 0x7f) {
          buf[0] = '\\';
          buf[1] = (char)('0' + ((c >> 6) & 7));
          buf[2] = (char)('0' + ((c >> 3) & 7));
          buf[3] = (char)('0' + (c & 7));
          plen = 4;
        } else {
          buf[0] = (char)c;
          plen = 1;
        }
    }

    if(col + plen > SED_LIST_WRAP) {
      st->out(st->ctx, "\\\n", 2);
      col = 0;
    }

    st->out(st->ctx, piece, plen);
    col += plen;
  }

  st->out(st->ctx, "$\n", 2);
}
