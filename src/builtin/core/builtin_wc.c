#include "builtin_config.h"

#if BUILTIN_WC

#include "../../builtin.h"
#include "../../fdtable.h"
#include "../../sh.h"
#include "../../../lib/shell.h"
#include "../../../lib/str.h"
#include "../../../lib/fmt.h"
#include "../../../lib/open.h"
#include "../../../lib/byte.h"
#include "../../../lib/utf8.h"
#include <sys/stat.h>

const char help_wc[] = "    Print newline, word, and byte counts.\n"
                       "\n"
                       "    -c, --bytes            print the byte counts\n"
                       "    -m, --chars            print the character counts (UTF-8 when LC_ALL, LC_CTYPE or\n"
                       "                           LANG names it, else bytes)\n"
                       "    -l, --lines            print the newline counts\n"
                       "    -L, --max-line-length  print the maximum display width\n"
                       "    -w, --words            print the word counts\n"
                       "    file                   file to count; '-' or omitted means stdin\n";

struct wc_counts {
  unsigned long lines, words, chars, bytes, maxlen;
};

/* counts lines/words/chars/bytes/longest-line in 'path' ("-" = stdin).
 * With utf8 set a character is a valid UTF-8 sequence, or one byte when the
 * bytes are not valid (the count u8count() gives); else chars == bytes.
 * Returns 0 on success, -1 on open/read failure.
 *
 *   int   utf8   count characters as UTF-8 sequences
 * ----------------------------------------------------------------------- */
static int
wc_count(const char* path, int utf8, struct wc_counts* out) {
  buffer inb;
  buffer* in;
  char rbuf[4096];
  char pend[4]; /* bytes of a sequence that may still be completed */
  size_t np = 0;
  int in_word = 0, eof = 0;
  unsigned long curlen = 0;
  char c;
  ssize_t r;

  byte_zero(out, sizeof(*out));

  if(!str_diff(path, "-")) {
    in = fd_in->r;
  } else {
    int rfd = open_read(path);

    if(rfd == -1)
      return -1;

    in = &inb;
    buffer_init(in, &buffer_op_read, rfd, rbuf, sizeof(rbuf));
  }

  /* one byte at a time: each byte is counted, then the character(s) it
     completes are decided (the whole of pend at eof) */
  for(;;) {
    if(!eof) {
      if((r = buffer_getc(in, &c)) <= 0)
        eof = 1;
      else
        out->bytes++, pend[np++] = c;
    }

    if(eof && !np)
      break;

    while(np) {
      unsigned cp;
      int l = utf8 ? u8decode(pend, np, &cp) : 1;
      char first = pend[0];

      if(l == -2 && !eof)
        break;

      l = l > 0 ? l : 1;
      out->chars++;
      for(size_t k = 0; k + (size_t)l < np; k++)
        pend[k] = pend[k + (size_t)l];

      np -= (size_t)l;

      if(first == '\n') {
        out->lines++;

        if(curlen > out->maxlen)
          out->maxlen = curlen;

        curlen = 0;
      } else {
        curlen++;
      }

      if(first == ' ' || first == '\t' || first == '\n' || first == '\v' || first == '\f' || first == '\r') {
        in_word = 0;
      } else if(!in_word) {
        in_word = 1;
        out->words++;
      }
    }
  }

  if(curlen > out->maxlen)
    out->maxlen = curlen;

  return r < 0 ? -1 : 0;
}

/* prints the columns selected by opt_* for 'c', right-justified to 'width'
 * (7, or 1 when a single count is the whole output), in wc's fixed column order (lines, words, chars,
 * bytes, max-line-length) regardless of the order options were given.
 * ----------------------------------------------------------------------- */
static void
wc_print(struct wc_counts* c, int opt_l, int opt_w, int opt_m, int opt_c, int opt_L, int width) {
  char buf[FMT_ULONG];
  ssize_t n;
  int first = 1;

#define WC_FIELD(cond, value) \
  if(cond) { \
    if(!first) \
      buffer_putspace(fd_out->w); \
    first = 0; \
    n = fmt_ulong(buf, (value)); \
    if(n < width) \
      buffer_putnspace(fd_out->w, width - n); \
    buffer_put(fd_out->w, buf, n); \
  }

  WC_FIELD(opt_l, c->lines)
  WC_FIELD(opt_w, c->words)
  WC_FIELD(opt_m, c->chars)
  WC_FIELD(opt_c, c->bytes)
  WC_FIELD(opt_L, c->maxlen)
#undef WC_FIELD
}

int
builtin_wc(int argc, char* argv[]) {
  int c, opt_c = 0, opt_m = 0, opt_l = 0, opt_L = 0, opt_w = 0, ret = 0, i, nfiles, width, implicit = 0;
  struct wc_counts total;
  int utf8 = sh_utf8();

  while((c = shell_getopt(argc, argv, "cmlLw")) > 0) {
    switch(c) {
      case 'c': opt_c = 1; break;
      case 'm': opt_m = 1; break;
      case 'l': opt_l = 1; break;
      case 'L': opt_L = 1; break;
      case 'w': opt_w = 1; break;
      default: builtin_invopt(argv); return 1;
    }
  }

  if(!(opt_c || opt_m || opt_l || opt_L || opt_w))
    opt_l = opt_w = opt_c = 1;

  if(shell_optind >= argc) {
    argv[argc++] = "-";
    implicit = 1;
  }

  nfiles = argc - shell_optind;

  /* like GNU: a lone count is bare, files get the width of their summed size, anything else 7 */
  width = 7;

  if(opt_l + opt_w + opt_m + opt_c + opt_L == 1 && nfiles == 1) {
    width = 1;
  } else {
    unsigned long long sum = 0;

    for(i = shell_optind; i < argc; i++) {
      struct stat st;

      if(str_equal(argv[i], "-") || stat(argv[i], &st) != 0 || !S_ISREG(st.st_mode))
        break;

      sum += (unsigned long long)st.st_size;
    }

    if(i == argc) {
      char nb[FMT_ULONG];

      width = (int)fmt_ulonglong(nb, sum);
    }
  }

  byte_zero(&total, sizeof(total));

  for(i = shell_optind; i < argc; i++) {
    struct wc_counts cnt;

    if(wc_count(argv[i], utf8, &cnt) == -1) {
      builtin_error(argv, argv[i]);
      ret = 1;
      continue;
    }

    wc_print(&cnt, opt_l, opt_w, opt_m, opt_c, opt_L, width);

    if(!implicit) { /* an operand "-" is named too; no operand at all is not */
      buffer_putspace(fd_out->w);
      buffer_puts(fd_out->w, argv[i]);
    }

    buffer_putnlflush(fd_out->w);

    total.lines += cnt.lines;
    total.words += cnt.words;
    total.chars += cnt.chars;
    total.bytes += cnt.bytes;

    if(cnt.maxlen > total.maxlen)
      total.maxlen = cnt.maxlen;
  }

  if(nfiles > 1) {
    wc_print(&total, opt_l, opt_w, opt_m, opt_c, opt_L, width);
    buffer_putspace(fd_out->w);
    buffer_puts(fd_out->w, "total");
    buffer_putnlflush(fd_out->w);
  }

  return ret;
}
#endif /* BUILTIN_WC */
