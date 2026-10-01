#include "../../builtin.h"
#include "../../fdtable.h"
#include "../../sh.h"
#include "../../../lib/shell.h"
#include "../../../lib/fmt.h"
#include "../../../lib/byte.h"
#include "../../../lib/utf8.h"
#include "../../../lib/stralloc.h"

const char help_uniq[] = "    Report or filter out repeated lines.\n"
                         "\n"
                         "    -c              precede each line by the number of times it occurred\n"
                         "    -d              only print lines that are repeated (one copy each)\n"
                         "    -u              only print lines that are not repeated\n"
                         "    -f fields       ignore the first fields blank-separated fields when comparing\n"
                         "    -s chars        then ignore the first chars characters (UTF-8 when the\n"
                         "                    locale variables name it, else bytes)\n"
                         "    input_file      file to read; '-' or omitted means stdin\n"
                         "    output_file     file to write instead of standard output\n";

struct uniq {
  struct filter_in in;
  unsigned long nfields, nchars, count;
  unsigned c : 1, d : 1, u : 1, utf8 : 1, have : 1;
  char* files[2]; /* the operands that are input files */
  char* outname;  /* the second operand */
  stralloc prev, out;
};

static int
uniq_option(void* ctx, int ch) {
  struct uniq* q = ctx;

  switch(ch) {
    case 'c': q->c = 1; return 0;
    case 'd': q->d = 1; return 0;
    case 'u': q->u = 1; return 0;

    case 'f':
      if(filter_opt_count(shell_optarg, &q->nfields) < 0) {
        q->in.err_arg = shell_optarg;
        q->in.err_msg = "invalid number of fields to skip";
        return -1;
      }

      return 0;

    case 's':
      if(filter_opt_count(shell_optarg, &q->nchars) < 0) {
        q->in.err_arg = shell_optarg;
        q->in.err_msg = "invalid number of characters to skip";
        return -1;
      }

      return 0;
  }

  return -1;
}

/* input_file and output_file are the only operands; the filter reads the first */
static int
uniq_setup(void* ctx) {
  struct uniq* q = ctx;

  q->utf8 = sh_utf8();

  if(q->in.files) {
    if(q->in.files[0] && q->in.files[1] && q->in.files[2]) {
      q->in.err_arg = q->in.files[2];
      q->in.err_msg = "extra operand";
      return -1;
    }

    q->files[0] = q->in.files[0];
    q->outname = q->in.files[1];
    q->in.files = q->files;
  }

  return 0;
}

static const char*
uniq_output(void* ctx) {
  return ((struct uniq*)ctx)->outname;
}

/* where the compared part of a line starts: after nfields fields (blanks then
 * non-blanks), then nchars characters */
static size_t
uniq_key(struct uniq* q, const char* s, size_t n) {
  size_t i = 0;
  unsigned long f;

  for(f = 0; f < q->nfields; f++) {
    while(i < n && (s[i] == ' ' || s[i] == '\t'))
      i++;

    while(i < n && s[i] != ' ' && s[i] != '\t')
      i++;
  }

  return i + text_charskip(q->utf8, s + i, n - i, q->nchars);
}

/* the finished group in q->prev as output, if the options let it through */
static int
uniq_format(struct uniq* q) {
  if(q->d || q->u ? !((q->d && q->count > 1) || (q->u && q->count == 1)) : 0)
    return 0;

  q->out.len = 0;

  if(q->c) {
    char buf[FMT_ULONG];
    size_t n = fmt_ulong(buf, q->count);

    while(n++ < 7)
      stralloc_catc(&q->out, ' ');

    stralloc_catb(&q->out, buf, fmt_ulong(buf, q->count));
    stralloc_catc(&q->out, ' ');
  }

  stralloc_catb(&q->out, q->prev.s, q->prev.len);
  stralloc_catc(&q->out, '\n');
  return 1;
}

static int
uniq_step(void* arg, const char** unit, size_t* len) {
  struct uniq* q = arg;
  const char* line;
  int had_nl;
  ssize_t r;

  for(;;) {
    int emit;

    if((r = filter_in_line(&q->in, &line, &had_nl)) < 0) {
      emit = q->have && uniq_format(q);
      q->have = 0;

      if(!emit)
        return 0;

      *unit = q->out.s;
      *len = q->out.len;
      return 1;
    }

    if(q->have) {
      size_t a = uniq_key(q, q->prev.s, q->prev.len), b = uniq_key(q, line, (size_t)r);

      if(q->prev.len - a == (size_t)r - b && !byte_diff(q->prev.s + a, q->prev.len - a, line + b)) {
        q->count++;
        continue;
      }
    }

    emit = q->have && uniq_format(q);
    q->prev.len = 0;
    stralloc_catb(&q->prev, line, (size_t)r);
    q->count = 1;
    q->have = 1;

    if(emit) {
      *unit = q->out.s;
      *len = q->out.len;
      return 1;
    }
  }
}

static void
uniq_finish(void* ctx) {
  struct uniq* q = ctx;

  stralloc_free(&q->prev);
  stralloc_free(&q->out);
}

const struct filter_ops uniq_ops = {
    .opts = "cduf:s:",
    .size = sizeof(struct uniq),
    .option = uniq_option,
    .setup = uniq_setup,
    .step = uniq_step,
    .finish = uniq_finish,
    .output = uniq_output,
};

FILTER_BUILTIN(uniq)