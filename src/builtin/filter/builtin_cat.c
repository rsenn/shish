#include "builtin_config.h"

#if BUILTIN_CAT

#include "../../builtin.h"
#include "../../fdtable.h"
#include "../../../lib/shell.h"
#include "../../../lib/fmt.h"
#include "../../../lib/byte.h"
#include "../../../lib/alloc.h"

const char help_cat[] = "    Concatenate files to standard output.\n"
                        "\n"
                        "    -n              number every output line\n"
                        "    -b              number only non-empty output lines\n"
                        "    -s              squeeze runs of empty lines into one\n"
                        "    -E              mark each line end with '$'\n"
                        "    -T              show tabs as ^I\n"
                        "    -v              show control characters as ^X and M-x (not tab or newline)\n"
                        "    -A -e -t        -vET, -vE, -vT\n"
                        "    -u              unbuffered output (accepted; output is not delayed)\n"
                        "    file            file to print; '-' or omitted means stdin\n";

/* one cat run, shared by the builtin and the filter (TODO.md Goal 13):
 * each step reads one "\r\n"-terminated unit and formats it. */
struct cat {
  struct filter_in in;
  int number_lines, number_nonempty;
  unsigned squeeze : 1, ends : 1, tabs : 1, vis : 1, bol : 1, blank : 1; /* bol: at a line start; blank: last line was empty */
  unsigned long line;
  char raw[1024];
  char linebuf[4 * 1024 + 48]; /* raw plus the number prefix, ^X forms and '$' */
};

static int
cat_step(void* arg, const char** unit, size_t* len) {
  struct cat* c = arg;
  ssize_t r = filter_in_get(&c->in, c->raw, sizeof(c->raw), "\r\n", 2);
  char nbuf[FMT_ULONG];
  size_t pos = 0, nn;

  if(r <= 0)
    return 0;

  c->in.newfile = 0;

  if(c->squeeze && c->bol && r == 1 && c->raw[0] == '\n') {
    if(c->blank)
      return cat_step(arg, unit, len);

    c->blank = 1;
  } else if(c->bol) {
    c->blank = 0;
  }

  *unit = c->raw;
  *len = (size_t)r;

  if(c->bol && (c->number_lines || (c->number_nonempty && r > 1))) {
    nn = fmt_ulong(nbuf, c->line);

    if(nn < 5) {
      byte_copy(c->linebuf, 5 - nn, "     ");
      pos = 5 - nn;
    }

    byte_copy(c->linebuf + pos, nn, nbuf);
    pos += nn;
    c->linebuf[pos++] = ' ';
    *unit = c->linebuf;
    c->line++;
  }

  if(c->ends || c->tabs || c->vis) {
    ssize_t i;

    for(i = 0; i < r; i++) {
      unsigned char ch = (unsigned char)c->raw[i];

      if(ch == '\n') {
        if(c->ends)
          c->linebuf[pos++] = '$';
      } else if(ch == '\t' ? c->tabs : c->vis && (ch < 32 || ch >= 127)) {
        if(ch >= 128) {
          c->linebuf[pos++] = 'M';
          c->linebuf[pos++] = '-';
          ch -= 128;
        }

        if(ch == 127 || ch < 32) {
          c->linebuf[pos++] = '^';
          ch = ch == 127 ? '?' : ch + 64;
        }
      }

      c->linebuf[pos++] = (char)ch;
    }

    *unit = c->linebuf;
    *len = pos;
  } else if(*unit == c->linebuf) {
    byte_copy(c->linebuf + pos, (size_t)r, c->raw);
    *len = pos + (size_t)r;
  }

  c->bol = c->raw[r - 1] == '\n';

  return 1;
}

static int
cat_option(void* ctx, int ch) {
  struct cat* c = ctx;

  switch(ch) {
    case 'n': c->number_lines = 1; return 0;
    case 'b': c->number_nonempty = 1; return 0;
    case 's': c->squeeze = 1; return 0;
    case 'E': c->ends = 1; return 0;
    case 'T': c->tabs = 1; return 0;
    case 'v': c->vis = 1; return 0;
    case 'A': c->vis = c->ends = c->tabs = 1; return 0;
    case 'e': c->vis = c->ends = 1; return 0;
    case 't': c->vis = c->tabs = 1; return 0;
    case 'u': return 0; /* output is flushed per line already */
    default: return -1;
  }
}

static int
cat_setup(void* ctx) {
  ((struct cat*)ctx)->line = 1;
  ((struct cat*)ctx)->bol = 1;
  return 0;
}

const struct filter_ops cat_ops = {
    .opts = "nbsETvAetu",
    .size = sizeof(struct cat),
    .option = cat_option,
    .setup = cat_setup,
    .step = cat_step,
};
const struct builtin_filter cat_filter = {&cat_ops};

int
builtin_cat(int argc, char* argv[]) {
  return filter_run(&cat_ops, argc, argv, fd_out->w);
}
#endif /* BUILTIN_CAT */
