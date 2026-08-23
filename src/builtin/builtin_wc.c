#include "../builtin.h"
#include "../fdtable.h"
#include "../../lib/shell.h"
#include "../../lib/str.h"
#include "../../lib/fmt.h"
#include "../../lib/open.h"
#include "../../lib/byte.h"

const char help_wc[] = "    Print newline, word, and byte counts.\n"
                       "\n"
                       "    -c, --bytes            print the byte counts\n"
                       "    -m, --chars            print the character counts\n"
                       "    -l, --lines            print the newline counts\n"
                       "    -L, --max-line-length  print the maximum display width\n"
                       "    -w, --words            print the word counts\n"
                       "    file                   file to count; '-' or omitted means stdin\n";

struct wc_counts {
  unsigned long lines, words, chars, bytes, maxlen;
};

/* counts lines/words/chars/bytes/longest-line in 'path' ("-" =
 * stdin), byte-at-a-time (no multibyte decoding: chars == bytes).
 * Returns 0 on success, -1 on open/read failure.
 * ----------------------------------------------------------------------- */
static int
wc_count(const char* path, struct wc_counts* out) {
  buffer inb;
  buffer* in;
  char rbuf[4096];
  int in_word = 0;
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

  while((r = buffer_getc(in, &c)) > 0) {
    out->bytes++;
    out->chars++;

    if(c == '\n') {
      out->lines++;

      if(curlen > out->maxlen)
        out->maxlen = curlen;

      curlen = 0;
    } else {
      curlen++;
    }

    if(c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r') {
      in_word = 0;
    } else if(!in_word) {
      in_word = 1;
      out->words++;
    }
  }

  if(curlen > out->maxlen)
    out->maxlen = curlen;

  return r < 0 ? -1 : 0;
}

/* prints the columns selected by opt_* for 'c', right-justified to a
 * fixed width, in wc's fixed column order (lines, words, chars,
 * bytes, max-line-length) regardless of the order options were given.
 * ----------------------------------------------------------------------- */
static void
wc_print(struct wc_counts* c, int opt_l, int opt_w, int opt_m, int opt_c, int opt_L) {
  char buf[FMT_ULONG];
  ssize_t n;

#define WC_FIELD(cond, value) \
  if(cond) { \
    n = fmt_ulong(buf, (value)); \
    if(n < 7) \
      buffer_putnspace(fd_out->w, 7 - n); \
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
  int c, opt_c = 0, opt_m = 0, opt_l = 0, opt_L = 0, opt_w = 0, ret = 0, i, nfiles;
  struct wc_counts total;

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
  }

  nfiles = argc - shell_optind;
  byte_zero(&total, sizeof(total));

  for(i = shell_optind; i < argc; i++) {
    struct wc_counts cnt;

    if(wc_count(argv[i], &cnt) == -1) {
      builtin_error(argv, argv[i]);
      ret = 1;
      continue;
    }

    wc_print(&cnt, opt_l, opt_w, opt_m, opt_c, opt_L);

    if(str_diff(argv[i], "-")) {
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
    wc_print(&total, opt_l, opt_w, opt_m, opt_c, opt_L);
    buffer_putspace(fd_out->w);
    buffer_puts(fd_out->w, "total");
    buffer_putnlflush(fd_out->w);
  }

  return ret;
}
