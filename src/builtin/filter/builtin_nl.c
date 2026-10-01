#include "../../builtin.h"
#include "../../fdtable.h"
#include "../../../lib/shell.h"
#include "../../../lib/fmt.h"
#include "../../../lib/str.h"
#include "../../../lib/stralloc.h"
#include "../../../text/dfa.h"

const char help_nl[] = "    Number lines of files.\n"
                       "\n"
                       "    -b type         number the body lines: a (all), t (non-empty, default), n (none), pRE\n"
                       "    -f type         same for footer lines (default n)\n"
                       "    -h type         same for header lines (default n)\n"
                       "    -d delim        section delimiter, one or two characters (default \\:)\n"
                       "    -i incr         line number increment (default 1)\n"
                       "    -l num          count num empty lines as one (with type a)\n"
                       "    -n format       ln, rn or rz: left, right or zero-padded number (default rn)\n"
                       "    -p              do not restart numbering at each logical page\n"
                       "    -s sep          separator between number and line (default tab)\n"
                       "    -v start        first line number (default 1)\n"
                       "    -w width        number width (default 6)\n"
                       "    file            file to read; '-' or omitted means stdin\n";

/* which lines a section numbers */
struct nl_type {
  char t; /* 'a' 't' 'n' or 'p' */
  struct dfa re;
};

struct nl {
  struct filter_in in;
  struct nl_type type[3]; /* header, body, footer */
  int sec;                /* section in progress: 0 header, 1 body, 2 footer */
  const char* sep;
  char delim[2];
  long start, incr, num;
  unsigned long width, join, blanks;
  char fmt; /* 'l' left, 'r' right, 'z' zero-padded */
  unsigned p : 1, have_start : 1, have_incr : 1;
  stralloc out;
};

static int
nl_type_parse(struct nl* n, struct nl_type* t, const char* s) {
  if((*s == 'a' || *s == 't' || *s == 'n') && !s[1]) {
    t->t = *s;
    return 0;
  }

  if(*s == 'p' && dfa_compile(&t->re, s + 1, str_len(s + 1), 0) == DFA_OK) {
    t->t = 'p';
    return 0;
  }

  n->in.err_arg = s;
  n->in.err_msg = "invalid line numbering type";
  return -1;
}

static int
nl_number(struct nl* n, unsigned long* out, const char* s, const char* what) {
  if(filter_opt_count(s, out) >= 0)
    return 0;

  n->in.err_arg = s;
  n->in.err_msg = what;
  return -1;
}

static int
nl_option(void* ctx, int ch) {
  struct nl* n = ctx;
  unsigned long v;

  switch(ch) {
    case 'h': return nl_type_parse(n, &n->type[0], shell_optarg);
    case 'b': return nl_type_parse(n, &n->type[1], shell_optarg);
    case 'f': return nl_type_parse(n, &n->type[2], shell_optarg);
    case 'p': n->p = 1; return 0;
    case 's': n->sep = shell_optarg; return 0;

    case 'd':
      if(!shell_optarg[0] || (shell_optarg[1] && shell_optarg[2])) {
        n->in.err_arg = shell_optarg;
        n->in.err_msg = "invalid section delimiter";
        return -1;
      }

      n->delim[0] = shell_optarg[0];
      n->delim[1] = shell_optarg[1] ? shell_optarg[1] : ':';
      return 0;

    case 'n':
      if(!str_equal(shell_optarg, "ln") && !str_equal(shell_optarg, "rn") && !str_equal(shell_optarg, "rz")) {
        n->in.err_arg = shell_optarg;
        n->in.err_msg = "invalid line numbering format";
        return -1;
      }

      n->fmt = shell_optarg[1] == 'n' ? shell_optarg[0] : 'z';
      return 0;

    case 'w':
      if(nl_number(n, &v, shell_optarg, "invalid line number field width") < 0 || !v)
        return -1;

      n->width = v;
      return 0;

    case 'l':
      if(nl_number(n, &v, shell_optarg, "invalid line number of blank lines") < 0 || !v)
        return -1;

      n->join = v;
      return 0;

    case 'i':
    case 'v': {
      const char* s = shell_optarg;
      int neg = *s == '-';

      if(neg || *s == '+')
        s++;

      if(nl_number(n, &v, s, ch == 'i' ? "invalid line number increment" : "invalid starting line number") < 0)
        return -1;

      if(ch == 'i')
        n->incr = neg ? -(long)v : (long)v, n->have_incr = 1;
      else
        n->start = neg ? -(long)v : (long)v, n->have_start = 1;

      return 0;
    }
  }

  return -1;
}

static int
nl_setup(void* ctx) {
  struct nl* n = ctx;

  /* the zeroed ctx stands for "option not given" */
  if(!n->type[0].t)
    n->type[0].t = 'n';
  if(!n->type[1].t)
    n->type[1].t = 't';
  if(!n->type[2].t)
    n->type[2].t = 'n';
  if(!n->sep)
    n->sep = "\t";
  if(!n->delim[0])
    n->delim[0] = '\\', n->delim[1] = ':';
  if(!n->fmt)
    n->fmt = 'r';
  if(!n->width)
    n->width = 6;
  if(!n->have_start)
    n->start = 1;
  if(!n->have_incr)
    n->incr = 1;

  n->num = n->start;
  n->sec = 1;
  return 0;
}

/* section delimiter line: delim x 3 header, x 2 body, x 1 footer; -1 if the line is none */
static int
nl_section(struct nl* n, const char* s, size_t len) {
  size_t k;

  if(len % 2 || len < 2 || len > 6)
    return -1;

  for(k = 0; k < len; k++)
    if(s[k] != n->delim[k & 1])
      return -1;

  return 3 - (int)(len / 2);
}

/* should this line get a number in the current section? */
static int
nl_wanted(struct nl* n, const char* s, size_t len) {
  struct nl_type* t = &n->type[n->sec];

  switch(t->t) {
    case 'a':
      if(len || n->join <= 1)
        return n->blanks = 0, 1;

      if(++n->blanks < n->join)
        return 0;

      return n->blanks = 0, 1;
    case 't': return len > 0;
    case 'p': return dfa_test(&t->re, s, len) != 0;
  }

  return 0;
}

static int
nl_step(void* arg, const char** unit, size_t* len) {
  struct nl* n = arg;
  const char* line;
  int had_nl, sec;
  ssize_t r;

  if((r = filter_in_line(&n->in, &line, &had_nl)) < 0)
    return 0;

  n->out.len = 0;

  if((sec = nl_section(n, line, (size_t)r)) >= 0) {
    if(!n->p) /* every delimiter line restarts, as GNU nl does */
      n->num = n->start;

    n->sec = sec;
    n->blanks = 0;
    stralloc_catc(&n->out, '\n');
  } else if(nl_wanted(n, line, (size_t)r)) {
    char buf[FMT_ULONG];
    size_t l = fmt_ulong(buf, n->num < 0 ? -n->num : n->num), pad = n->width;
    size_t used = l + (n->num < 0);

    pad = used < pad ? pad - used : 0;

    if(n->fmt == 'r')
      while(pad--)
        stralloc_catc(&n->out, ' ');

    if(n->num < 0)
      stralloc_catc(&n->out, '-');

    if(n->fmt == 'z')
      while(pad--)
        stralloc_catc(&n->out, '0');

    stralloc_catb(&n->out, buf, l);

    if(n->fmt == 'l')
      while(pad--)
        stralloc_catc(&n->out, ' ');

    stralloc_cats(&n->out, n->sep);
    stralloc_catb(&n->out, line, r);
    stralloc_catc(&n->out, '\n');
    n->num += n->incr;
  } else {
    size_t i;

    for(i = 0; i < n->width + str_len(n->sep); i++)
      stralloc_catc(&n->out, ' ');

    stralloc_catb(&n->out, line, r);
    stralloc_catc(&n->out, '\n');
  }

  *unit = n->out.s;
  *len = n->out.len;
  return 1;
}

static int
nl_status(void* ctx) {
  return ((struct nl*)ctx)->in.had_error;
}

static void
nl_finish(void* ctx) {
  struct nl* n = ctx;
  int i;

  for(i = 0; i < 3; i++)
    if(n->type[i].t == 'p')
      dfa_free(&n->type[i].re);

  stralloc_free(&n->out);
}

const struct filter_ops nl_ops = {
    .opts = "b:d:f:h:i:l:n:ps:v:w:",
    .size = sizeof(struct nl),
    .option = nl_option,
    .setup = nl_setup,
    .step = nl_step,
    .status = nl_status,
    .finish = nl_finish,
};

FILTER_BUILTIN(nl)