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
  struct filter_out out;
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

/* parses -n/-b and sets up the run; -1 on a bad option (nothing printed) */
static int
cat_init(struct cat* c, int argc, char* argv[], buffer* upstream) {
  int ch;

  byte_zero(c, sizeof(*c));

  while((ch = shell_getopt(argc, argv, "nb")) > 0) {
    switch(ch) {
      case 'n': c->number_lines = 1; break;
      case 'b': c->number_nonempty = 1; break;
      default: return -1;
    }
  }

  filter_in_init(&c->in, argv, argv[shell_optind] ? argv + shell_optind : NULL, upstream);
  c->line = 1;
  return 0;
}

int
builtin_cat(int argc, char* argv[]) {
  struct cat c;
  const char* unit;
  size_t len;
  int ret;

  if(cat_init(&c, argc, argv, fd_in->r) == -1) {
    builtin_invopt(argv);
    return 1;
  }

  while(cat_step(&c, &unit, &len)) {
    buffer_put(fd_out->w, unit, len);
    buffer_flush(fd_out->w);
  }

  ret = c.in.had_error;
  filter_in_close(&c.in);
  return ret;
}

static ssize_t
cat_filter_read(int fd, void* buf, size_t len, void* arg) {
  (void)fd;
  return filter_out_read(&((struct cat*)arg)->out, buf, len, cat_step, arg);
}

static int
cat_filter_status(void* arg) {
  return ((struct cat*)arg)->in.had_error;
}

static void
cat_filter_close(void* arg) {
  filter_in_close(&((struct cat*)arg)->in);
  alloc_free(arg);
}

/* a bad option returns NULL without printing: builtin_cat() reports it */
static void*
cat_filter_open(int argc, char* argv[], buffer* upstream) {
  struct cat* c = alloc(sizeof(*c));

  if(c && cat_init(c, argc, argv, upstream) == -1) {
    alloc_free(c);
    c = NULL;
  }

  return c;
}

const struct filter_ops cat_ops = {cat_filter_open, cat_filter_read, cat_filter_status, cat_filter_close};
const struct builtin_filter cat_filter = {&cat_ops};
