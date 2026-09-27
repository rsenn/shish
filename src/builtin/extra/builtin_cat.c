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
                        "    file            file to print; '-' or omitted means stdin\n";

/* one cat run, shared by the builtin and the filter (TODO.md Goal 13):
 * each step reads one "\r\n"-terminated unit and formats it. */
struct cat {
  struct filter_in in;
  int number_lines, number_nonempty;
  unsigned long line;
  char raw[1024];
  char linebuf[1040]; /* raw plus the -n/-b number prefix */
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
  *unit = c->raw;
  *len = (size_t)r;

  if(c->number_lines || (c->number_nonempty && r > 1)) {
    nn = fmt_ulong(nbuf, c->line);

    if(nn < 5) {
      byte_copy(c->linebuf, 5 - nn, "     ");
      pos = 5 - nn;
    }

    byte_copy(c->linebuf + pos, nn, nbuf);
    pos += nn;
    c->linebuf[pos++] = ' ';
    byte_copy(c->linebuf + pos, (size_t)r, c->raw);
    *unit = c->linebuf;
    *len = pos + (size_t)r;
  }

  if(c->raw[r - 1] == '\n')
    c->line++;

  return 1;
}

static int
cat_option(void* ctx, int ch) {
  struct cat* c = ctx;

  switch(ch) {
    case 'n': c->number_lines = 1; return 0;
    case 'b': c->number_nonempty = 1; return 0;
    default: return -1;
  }
}

static int
cat_setup(void* ctx) {
  ((struct cat*)ctx)->line = 1;
  return 0;
}

const struct filter_ops cat_ops = {.opts = "nb", .size = sizeof(struct cat), .option = cat_option, .setup = cat_setup, .step = cat_step};
const struct builtin_filter cat_filter = {&cat_ops};

int
builtin_cat(int argc, char* argv[]) {
  return filter_run(&cat_ops, argc, argv, fd_out->w);
}
