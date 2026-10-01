#include "../../builtin.h"
#include "../../fdtable.h"
#include "../../sh.h"
#include "../../../lib/shell.h"
#include "../../../lib/alloc.h"
#include "../../../lib/byte.h"
#include "../../../lib/str.h"
#include "../../../lib/utf8.h"
#include "../../../lib/stralloc.h"

const char help_paste[] = "    Merge corresponding or subsequent lines of files.\n"
                          "\n"
                          "    -d list         delimiters used in turn between the merged lines (default tab);\n"
                          "                    \\n \\t \\\\ are understood, \\0 is an empty delimiter\n"
                          "    -s              paste one file's lines together, one output line per file\n"
                          "    file            file to read; '-' means stdin (all '-' share it, line by line)\n";

struct paste {
  struct filter_in in;          /* the operand list */
  struct filter_in *all, **src; /* an input per operand; src[i] -> all[k], '-' sharing one */
  char** names;                 /* {file, NULL} pairs the inputs read from */
  size_t n, nall, cur;          /* operands, inputs opened; -s: operand in progress */
  const char* dlist;            /* -d list, still escaped */
  stralloc dbuf;                /* delimiters, unescaped, back to back */
  size_t *doff, *dlen, nd;      /* where delimiter k lives in dbuf, and how long it is */
  unsigned s : 1, utf8 : 1;
  stralloc out;
};

static int
paste_option(void* ctx, int ch) {
  struct paste* p = ctx;

  switch(ch) {
    case 'd': p->dlist = shell_optarg; return 0;
    case 's': p->s = 1; return 0;
  }

  return -1;
}

/* "\t:\n" -> delimiters "\t", ":", "\n"; "\0" -> "" */
static int
paste_delims(struct paste* p, const char* s) {
  size_t len = str_len(s);

  if(!(p->doff = alloc(2 * (len + 1) * sizeof(size_t))))
    return -1;

  p->dlen = p->doff + len + 1;

  while(*s) {
    size_t l = 1;

    p->doff[p->nd] = p->dbuf.len;

    if(*s == '\\' && s[1]) {
      char e = *++s;

      if(e != '0')
        stralloc_catc(&p->dbuf, e == 'n' ? '\n' : e == 't' ? '\t' : e);
    } else {
      l = text_charlen(p->utf8, s, str_len(s));
      stralloc_catb(&p->dbuf, s, l);
    }

    p->dlen[p->nd] = p->dbuf.len - p->doff[p->nd];
    p->nd++;
    s += l;
  }

  return 0;
}

static void
paste_delim(struct paste* p, size_t k) {
  k %= p->nd;
  stralloc_catb(&p->out, p->dbuf.s + p->doff[k], p->dlen[k]);
}

static int
paste_setup(void* ctx) {
  struct paste* p = ctx;
  static char* dash[] = {"-", NULL};
  char** f = p->in.files ? p->in.files : dash;
  size_t i, k = 0;

  p->utf8 = sh_utf8();

  if(paste_delims(p, p->dlist ? p->dlist : "\\t") < 0)
    return -1;

  for(p->n = 0; f[p->n]; p->n++) {}

  p->all = alloc(p->n * sizeof(*p->all));
  p->src = alloc(p->n * sizeof(*p->src));
  p->names = alloc(2 * p->n * sizeof(*p->names));

  if(!p->all || !p->src || !p->names)
    return -1;

  for(i = 0; i < p->n; i++) {
    size_t j;

    for(j = 0; j < i && !(str_equal(f[i], "-") && str_equal(f[j], "-")); j++) {}

    if(j < i) {
      p->src[i] = p->src[j];
      continue;
    }

    p->names[2 * k] = f[i];
    p->names[2 * k + 1] = NULL;
    filter_in_init(&p->all[k], p->in.errargv, &p->names[2 * k], p->in.upstream);
    p->src[i] = &p->all[k++];
  }

  p->nall = k;

  return 0;
}

static int
paste_step(void* arg, const char** unit, size_t* len) {
  struct paste* p = arg;
  const char* line;
  int had_nl;
  ssize_t r;
  size_t i, alive = 0;

  p->out.len = 0;

  if(p->s) {
    size_t k = 0;

    if(p->cur >= p->n)
      return 0;

    while((r = filter_in_line(p->src[p->cur], &line, &had_nl)) >= 0) {
      if(k)
        paste_delim(p, k - 1);

      stralloc_catb(&p->out, line, r);
      k++;
    }

    p->cur++;
  } else {
    for(i = 0; i < p->n; i++) {
      if((r = filter_in_line(p->src[i], &line, &had_nl)) >= 0) {
        alive++;
        stralloc_catb(&p->out, line, r);
      }

      if(i + 1 < p->n)
        paste_delim(p, i);
    }

    if(!alive)
      return 0;
  }

  stralloc_catc(&p->out, '\n');
  *unit = p->out.s;
  *len = p->out.len;
  return 1;
}

static int
paste_status(void* ctx) {
  struct paste* p = ctx;
  size_t i;

  for(i = 0; i < p->n; i++)
    if(p->src[i]->had_error)
      return 1;

  return p->in.had_error;
}

static void
paste_finish(void* ctx) {
  struct paste* p = ctx;
  size_t i;

  for(i = 0; i < p->nall; i++)
    filter_in_close(&p->all[i]);

  alloc_free(p->all);
  alloc_free(p->src);
  alloc_free(p->names);
  alloc_free(p->doff);
  stralloc_free(&p->dbuf);
  stralloc_free(&p->out);
}

const struct filter_ops paste_ops = {
    .opts = "d:s",
    .size = sizeof(struct paste),
    .option = paste_option,
    .setup = paste_setup,
    .step = paste_step,
    .status = paste_status,
    .finish = paste_finish,
};

FILTER_BUILTIN(paste)