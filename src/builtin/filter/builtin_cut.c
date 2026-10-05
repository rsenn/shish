#include "builtin_config.h"

#if BUILTIN_CUT

#include "../../builtin.h"
#include "../../fdtable.h"
#include "../../sh.h"
#include "../../../lib/shell.h"
#include "../../../lib/alloc.h"
#include "../../../lib/byte.h"
#include "../../../lib/str.h"
#include "../../../lib/utf8.h"
#include "../../../lib/stralloc.h"

const char help_cut[] = "    Select parts of each line.\n"
                        "\n"
                        "    -b list         select these bytes\n"
                        "    -c list         select these characters (UTF-8 when the locale variables\n"
                        "                    name it, else bytes)\n"
                        "    -f list         select these delimiter-separated fields\n"
                        "    -d delim        the field delimiter, one character (default tab)\n"
                        "    -s              with -f: skip lines that contain no delimiter\n"
                        "    -n              with -b in a UTF-8 locale: never split a character\n"
                        "    file            file to read; '-' or omitted means stdin\n"
                        "\n"
                        "    list is N, N-M, N- or -M items separated by commas, counted from 1.\n";

#define CUT_MAX ((size_t)-1)

struct range {
  size_t lo, hi; /* 1-based, inclusive; hi may be CUT_MAX */
};

struct cut {
  struct filter_in in;
  struct range* r; /* sorted, disjoint after setup */
  size_t nr, cap;
  char mode; /* 'b' 'c' 'f' */
  char delim[4];
  size_t dlen;
  unsigned s : 1, hasd : 1, utf8 : 1, nosplit : 1;
  stralloc out;
};

/* one "N", "N-M", "N-" or "-M" item; 0 ok, -1 invalid */
static int
cut_item(struct cut* c, const char* s, size_t n) {
  size_t lo = 0, hi = 0, i = 0;
  int haslo = 0, hashi = 0, dash = 0;

  for(; i < n; i++) {
    if(s[i] == '-' && !dash) {
      dash = 1;
    } else if(s[i] >= '0' && s[i] <= '9') {
      size_t* v = dash ? &hi : &lo;

      if(*v > (CUT_MAX - 9) / 10)
        return -1;

      *v = *v * 10 + (size_t)(s[i] - '0');
      *(dash ? &hashi : &haslo) = 1;
    } else {
      return -1;
    }
  }

  if(!haslo && !hashi)
    return -1;

  if(!dash)
    hi = lo;
  else if(!haslo)
    lo = 1;
  else if(!hashi)
    hi = CUT_MAX;

  if(lo == 0 || hi < lo)
    return -1;

  if(c->nr == c->cap) {
    size_t cap = c->cap ? c->cap * 2 : 8;
    struct range* r = alloc_re(c->r, cap * sizeof(*r));

    if(!r)
      return -1;

    c->r = r;
    c->cap = cap;
  }

  c->r[c->nr].lo = lo;
  c->r[c->nr].hi = hi;
  c->nr++;
  return 0;
}

/* the whole list into c->r, then sorted and merged */
static int
cut_list(struct cut* c, const char* s) {
  size_t i, j;

  for(;;) {
    size_t n = 0;

    while(s[n] && s[n] != ',' && s[n] != ' ' && s[n] != '\t')
      n++;

    if(cut_item(c, s, n) < 0)
      return -1;

    s += n;

    if(!*s)
      break;

    s++;
  }

  for(i = 1; i < c->nr; i++) {
    struct range x = c->r[i];

    for(j = i; j > 0 && c->r[j - 1].lo > x.lo; j--)
      c->r[j] = c->r[j - 1];

    c->r[j] = x;
  }

  for(i = 1, j = 0; i < c->nr; i++) {
    if(c->r[i].lo <= c->r[j].hi || c->r[i].lo == c->r[j].hi + 1) {
      if(c->r[i].hi > c->r[j].hi)
        c->r[j].hi = c->r[i].hi;
    } else {
      c->r[++j] = c->r[i];
    }
  }

  c->nr = c->nr ? j + 1 : 0;
  return 0;
}

static int
cut_option(void* ctx, int ch) {
  struct cut* c = ctx;

  switch(ch) {
    case 'b':
    case 'c':
    case 'f':
      if(c->mode) {
        c->in.err_msg = "only one type of list may be specified";
        return -1;
      }

      c->mode = (char)ch;

      if(cut_list(c, shell_optarg) < 0) {
        c->in.err_arg = shell_optarg;
        c->in.err_msg = ch == 'f' ? "invalid field list" : ch == 'c' ? "invalid character list" : "invalid byte list";
        return -1;
      }

      return 0;

    case 'd':
      if(!shell_optarg[0] || u8charlen(shell_optarg, 4) != str_len(shell_optarg)) {
        c->in.err_arg = shell_optarg;
        c->in.err_msg = "the delimiter must be a single character";
        return -1;
      }

      c->dlen = u8charlen(shell_optarg, 4);
      byte_copy(c->delim, c->dlen, shell_optarg);
      c->hasd = 1;
      return 0;

    case 's': c->s = 1; return 0;
    case 'n': c->nosplit = 1; return 0;
  }

  return -1;
}

static int
cut_setup(void* ctx) {
  struct cut* c = ctx;

  if(!c->mode) {
    c->in.err_msg = "you must specify a list of bytes, characters, or fields";
    return -1;
  }

  if((c->hasd || c->s) && c->mode != 'f') {
    c->in.err_msg = "an input delimiter may be specified only when operating on fields";
    return -1;
  }

  if(!c->hasd) {
    c->delim[0] = '\t';
    c->dlen = 1;
  }

  c->utf8 = sh_utf8();
  return 0;
}

/* -n -b in UTF-8: a byte range never splits a character. "low" moves back to the first byte
 * of its character, "high" to the last byte of the previous one when it ends inside a character.
 * ----------------------------------------------------------------------- */
static void
cut_bytes_whole(struct cut* c, const char* line, size_t n) {
  size_t i, done = 0;

  for(i = 0; i < c->nr && done < n; i++) {
    size_t lo = c->r[i].lo, hi = c->r[i].hi < n ? c->r[i].hi : n, s, l;

    for(s = 0; s < n; s += l) {
      l = u8charlen(line + s, n - s);

      if(lo > s && lo <= s + l)
        lo = s + 1;

      if(hi > s && hi < s + l)
        hi = s;
    }

    if(lo <= done)
      lo = done + 1;

    if(hi >= lo) {
      stralloc_catb(&c->out, line + lo - 1, hi - lo + 1);
      done = hi;
    }
  }
}

/* -b/-c: the selected stretches of the line, in order */
static void
cut_chars(struct cut* c, const char* line, size_t n) {
  int utf8 = c->mode == 'c' && c->utf8;
  size_t off = 0, cpos = 0, i;

  if(c->mode == 'b' && c->nosplit && c->utf8) {
    cut_bytes_whole(c, line, n);
    return;
  }

  for(i = 0; i < c->nr && off < n; i++) {
    size_t lo = c->r[i].lo - 1, take;

    off += text_charskip(utf8, line + off, n - off, lo - cpos);
    cpos = lo;

    if(off >= n)
      break;

    if(c->r[i].hi == CUT_MAX) {
      take = n - off;
    } else {
      size_t count = c->r[i].hi - lo;

      take = text_charskip(utf8, line + off, n - off, count);
      cpos += count;
    }

    stralloc_catb(&c->out, line + off, take);
    off += take;
  }
}

/* -f: the selected delimiter-separated fields, joined by the delimiter */
static void
cut_fields(struct cut* c, const char* line, size_t n) {
  size_t start = 0, field = 1, r = 0, i;
  int first = 1;

  for(i = 0; i <= n; i++) {
    if(i < n && !(i + c->dlen <= n && !byte_diff(line + i, c->dlen, c->delim)))
      continue;

    while(r < c->nr && c->r[r].hi < field)
      r++;

    if(r < c->nr && c->r[r].lo <= field) {
      if(!first)
        stralloc_catb(&c->out, c->delim, c->dlen);

      stralloc_catb(&c->out, line + start, i - start);
      first = 0;
    }

    field++;
    start = i + c->dlen;
    i = start - 1;
  }
}

static int
cut_step(void* arg, const char** unit, size_t* len) {
  struct cut* c = arg;
  const char* line;
  int had_nl;
  ssize_t r;

  while((r = filter_in_line(&c->in, &line, &had_nl)) >= 0) {
    size_t n = (size_t)r, k;

    c->out.len = 0;

    if(c->mode == 'f') {
      /* no delimiter at all: the whole line, unless -s */
      for(k = 0; k + c->dlen <= n && byte_diff(line + k, c->dlen, c->delim); k++)
        ;

      if(k + c->dlen > n) {
        if(c->s)
          continue;

        stralloc_catb(&c->out, line, n);
      } else {
        cut_fields(c, line, n);
      }
    } else {
      cut_chars(c, line, n);
    }

    stralloc_catc(&c->out, '\n');
    *unit = c->out.s;
    *len = c->out.len;
    return 1;
  }

  return 0;
}

static void
cut_finish(void* ctx) {
  struct cut* c = ctx;

  alloc_free(c->r);
  stralloc_free(&c->out);
}

const struct filter_ops cut_ops = {
    .opts = "b:c:f:d:sn",
    .size = sizeof(struct cut),
    .option = cut_option,
    .setup = cut_setup,
    .step = cut_step,
    .finish = cut_finish,
};

FILTER_BUILTIN(cut)
#endif /* BUILTIN_CUT */
