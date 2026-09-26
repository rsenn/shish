#include "../history.h"
#include "../term.h"
#include "../../lib/windoze.h"

#if !WINDOWS_NATIVE
#include <poll.h>
#endif

/* minimal vi editing. insert mode is the normal line editor; ESC enters
 * command mode, where a key is a command read by term_vimode():
 *
 *   [n]h l 0 ^ $ w b e f F t T ; ,      motions
 *   [n]d c y <motion>  dd cc yy         operators (cw acts like ce)
 *   [n]x X D C s S r<c> p P             single-key edits
 *   i a I A                             back to insert mode
 *   k j /                               history up, down, search
 * ----------------------------------------------------------------------- */
int term_vi_cmd;

static stralloc vi_yank;
static char vi_fkey, vi_fchar; /* last f/F/t/T and its target, for ; and , */

static int
vi_class(char c) {
  if(c == ' ' || c == '\t')
    return 0;

  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' ? 1 : 2;
}

static int
vi_getc(char* c) {
  return buffer_getc(&term_input, c) > 0;
}

/* replaces the edit line by <n> and puts the cursor at <pos> */
static void
vi_set(stralloc* n, unsigned long pos) {
  term_setline(n->s, n->len);

  if(term_vi_cmd && pos >= n->len)
    pos = n->len ? n->len - 1 : 0;

  term_left(n->len - pos);
}

/* applies operator <op> (d, c or y) to the range [a, b) */
static void
vi_apply(char op, unsigned long a, unsigned long b) {
  stralloc n;

  if(b > term_cmdline.len)
    b = term_cmdline.len;

  if(a >= b)
    return;

  stralloc_zero(&vi_yank);
  stralloc_catb(&vi_yank, &term_cmdline.s[a], b - a);

  if(op == 'c')
    term_vi_cmd = 0;

  if(op == 'y') {
    term_left(term_pos - (a < term_pos ? a : term_pos));
    term_right(a > term_pos ? a - term_pos : 0);
    return;
  }

  stralloc_init(&n);
  stralloc_catb(&n, term_cmdline.s, a);
  stralloc_catb(&n, &term_cmdline.s[b], term_cmdline.len - b);
  vi_set(&n, a);
  stralloc_free(&n);
}

/* finds <ch> <cnt> times from <pos> in direction <dir>; -1 if not there */
static long
vi_find(char key, char ch, unsigned long cnt, unsigned long pos) {
  const char* s = term_cmdline.s;
  long len = term_cmdline.len, i = pos, dir = (key == 'f' || key == 't') ? 1 : -1;

  while(cnt--) {
    /* t/T repeated with ';' must not stay stuck on the char next to it */
    i += dir;

    while(i >= 0 && i < len && s[i] != ch)
      i += dir;

    if(i < 0 || i >= len)
      return -1;
  }

  if(key == 't')
    i--;
  else if(key == 'T')
    i++;

  return i;
}

/* target of motion <m> repeated <n> times, or -1; *incl is set when the
 * character under the target belongs to an operator's range */
static long
vi_motion(char m, unsigned long n, int* incl) {
  const char* s = term_cmdline.s;
  long len = term_cmdline.len, p = term_pos;
  char c;

  *incl = 0;

  switch(m) {
    case 'h': return p - (long)n < 0 ? 0 : p - (long)n;
    case 'l': return p + (long)n > len ? len : p + (long)n;
    case '0': return 0;
    case '^':
      for(p = 0; p < len && vi_class(s[p]) == 0; p++)
        ;
      return p;
    case '$': *incl = 1; return len ? len - 1 : 0;
    case 'w':
      while(n--) {
        int k = p < len ? vi_class(s[p]) : 0;

        while(p < len && k && vi_class(s[p]) == k)
          p++;
        while(p < len && vi_class(s[p]) == 0)
          p++;
      }
      return p;
    case 'b':
      while(n--) {
        if(p > 0)
          p--;
        while(p > 0 && vi_class(s[p]) == 0)
          p--;
        for(c = vi_class(s[p]); p > 0 && vi_class(s[p - 1]) == c;)
          p--;
      }
      return p;
    case 'e':
      *incl = 1;
      while(n--) {
        if(p + 1 < len)
          p++;
        while(p + 1 < len && vi_class(s[p]) == 0)
          p++;
        for(c = vi_class(s[p]); p + 1 < len && vi_class(s[p + 1]) == c;)
          p++;
      }
      return p;
    case 'f':
    case 'F':
    case 't':
    case 'T':
      if(!vi_getc(&c))
        return -1;
      vi_fkey = m;
      vi_fchar = c;
      *incl = m == 'f' || m == 't';
      return vi_find(m, c, n, p);
    case ';':
    case ',':
      if(!vi_fkey)
        return -1;
      c = vi_fkey;
      if(m == ',')
        c = c == 'f' ? 'F' : c == 'F' ? 'f' : c == 't' ? 'T' : 't';
      *incl = c == 'f' || c == 't';
      return vi_find(c, vi_fchar, n, p);
  }

  return -1;
}

/* reads a decimal count starting at <*c>; returns it (0 = none) */
static unsigned long
vi_count(char* c) {
  unsigned long n = 0;

  while((*c >= '1' && *c <= '9') || (n && *c == '0')) {
    n = n * 10 + (*c - '0');

    if(!vi_getc(c))
      break;
  }

  return n;
}

static void
vi_paste(int after) {
  stralloc n;
  unsigned long at = term_pos + (after && term_cmdline.len ? 1 : 0);

  if(!vi_yank.len)
    return;

  stralloc_init(&n);
  stralloc_catb(&n, term_cmdline.s, at);
  stralloc_catb(&n, vi_yank.s, vi_yank.len);
  stralloc_catb(&n, &term_cmdline.s[at], term_cmdline.len - at);
  vi_set(&n, at + vi_yank.len - 1);
  stralloc_free(&n);
}

/* leaves command mode; the cursor sits on a character, so one back */
static void
vi_command(void) {
  term_vi_cmd = 1;
  term_left(1);
}

/* ESC seen. 0 if it starts an arrow-key sequence for term_ansi(), else
 * it was a plain ESC and command mode is entered (a key typed right after
 * it, or none within 50ms, is the first command) */
int
term_vi_escape(void) {
  char c;

#if !WINDOWS_NATIVE
  if(term_input.p == term_input.n) {
    struct pollfd pfd = {term_input.fd, POLLIN, 0};

    if(poll(&pfd, 1, 50) <= 0) {
      vi_command();
      return 1;
    }
  }
#endif

  if(!vi_getc(&c))
    return 0;

  term_input.p--;

  if(c == '[' || c == 'O')
    return 0;

  vi_command();
  return 1;
}

void
term_vimode(char c) {
  unsigned long n1 = vi_count(&c), n2, cnt;
  unsigned long len = term_cmdline.len, pos = term_pos;
  char op = 0;
  long to;
  int incl;

  if(c == 'd' || c == 'c' || c == 'y') {
    op = c;

    if(!vi_getc(&c))
      return;

    n2 = vi_count(&c);
    cnt = (n1 ? n1 : 1) * (n2 ? n2 : 1);

    if(c == op) {
      vi_apply(op, 0, len);
      return;
    }

    if(op == 'c' && c == 'w' && pos < len && vi_class(term_cmdline.s[pos]))
      c = 'e';

    if((to = vi_motion(c, cnt, &incl)) < 0)
      return;

    if((unsigned long)to < pos)
      vi_apply(op, to, pos);
    else
      vi_apply(op, pos, to + incl);

    return;
  }

  cnt = n1 ? n1 : 1;

  if((to = vi_motion(c, cnt, &incl)) >= 0) {
    if(len && (unsigned long)to >= len)
      to = len - 1;

    if((unsigned long)to < pos)
      term_left(pos - to);
    else
      term_right(to - pos);

    return;
  }

  switch(c) {
    case 'x': vi_apply('d', pos, pos + cnt); break;
    case 'X': vi_apply('d', pos > cnt ? pos - cnt : 0, pos); break;
    case 'D': vi_apply('d', pos, len); break;
    case 'C': vi_apply('c', pos, len); break;
    case 's': vi_apply('c', pos, pos + cnt); break;
    case 'S': vi_apply('c', 0, len); break;
    case 'p': vi_paste(1); break;
    case 'P': vi_paste(0); break;
    case 'i': term_vi_cmd = 0; break;
    case 'a':
      term_vi_cmd = 0;
      term_right(1);
      break;
    case 'I':
      term_vi_cmd = 0;
      term_home();
      break;
    case 'A':
      term_vi_cmd = 0;
      term_end();
      break;
    case 'r':
      if(vi_getc(&c) && c >= ' ' && pos + cnt <= len) {
        stralloc n;
        unsigned long i;

        stralloc_init(&n);
        stralloc_copyb(&n, term_cmdline.s, len);

        for(i = 0; i < cnt; i++)
          n.s[pos + i] = c;

        vi_set(&n, pos + cnt - 1);
        stralloc_free(&n);
      }
      break;
    case 'k': history_prev(); break;
    case 'j': history_next(); break;
    case '/': term_search(); break;
  }
}
