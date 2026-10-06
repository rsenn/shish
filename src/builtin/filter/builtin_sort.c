#include "builtin_config.h"

#if BUILTIN_SORT

#include "../../builtin.h"
#include "../../fdtable.h"
#include "../../../lib/shell.h"
#include "../../../lib/alloc.h"
#include "../../../lib/byte.h"
#include "../../../lib/open.h"
#include "../../../lib/str.h"
#include "../../../lib/stralloc.h"
#include <unistd.h>

const char help_sort[] = "    Sort, merge or check the lines of files.\n"
                         "\n"
                         "    -c              check that the input is sorted; no output\n"
                         "    -s              stable: no last-resort whole-line comparison\n"
                         "    -m              merge presorted files (sorted like any other input here)\n"
                         "    -o file         write to file instead of standard output\n"
                         "    -u              keep only the first of lines with equal keys\n"
                         "    -b -d -f -i -n -r  ignore leading blanks; dictionary order; fold case;\n"
                         "                    ignore unprintable characters; numeric; reverse\n"
                         "    -t char         field separator (default: blank runs)\n"
                         "    -k field[.char][mods][,field[.char][mods]]\n"
                         "                    sort on a part of the line; mods are b d f i n r\n"
                         "    file            file to read; '-' or omitted means stdin\n"
                         "\n"
                         "    Characters compare as bytes (the C collating sequence).\n";

/* a -k key: where it starts and ends, and the modifiers attached to it */
struct sortkey {
  unsigned long fs, cs, fe, ce; /* field and character of the start and of the end */
  unsigned has_end : 1, typed : 1;
  unsigned bs : 1, be : 1, d : 1, f : 1, i : 1, n : 1, r : 1;
};

struct sortline {
  size_t off, len; /* into data; a newline follows the line there */
};

struct sort {
  struct filter_in in;
  unsigned c : 1, u : 1, stable : 1, loaded : 1, disorder : 1;
  struct sortkey glob; /* the global -b -d -f -i -n -r, in the same shape as a key's modifiers */
  int tsep;            /* -t character, -1: blanks */
  unsigned havet : 1;
  const char* outname;
  struct sortkey* keys;
  size_t nkeys, akeys;
  stralloc data, out, prev;
  struct sortline *lines, *tmp;
  size_t nlines, alines, next;
  unsigned long lineno;
};

static int
sort_blank(int c) {
  return c == ' ' || c == '\t';
}

/* "N[.C][bdfinr]" at *p; returns 0 and moves *p past it, or -1 */
static int
sort_keypart(const char** p, unsigned long* f, unsigned long* c, struct sortkey* k, int end) {
  const char* s = *p;
  unsigned long v;
  int digits = 0;

  v = 0;
  while(*s >= '0' && *s <= '9') {
    v = v * 10 + (unsigned long)(*s++ - '0');
    digits++;
  }

  if(!digits || (!end && v == 0))
    return -1;

  *f = v;
  *c = 0;

  if(*s == '.') {
    s++;
    v = 0;
    digits = 0;

    while(*s >= '0' && *s <= '9') {
      v = v * 10 + (unsigned long)(*s++ - '0');
      digits++;
    }

    if(!digits)
      return -1;

    *c = v;
  }

  for(;; s++) {
    switch(*s) {
      case 'b': if(end) k->be = 1; else k->bs = 1; break;
      case 'd': k->d = 1; break;
      case 'f': k->f = 1; break;
      case 'i': k->i = 1; break;
      case 'n': k->n = 1; break;
      case 'r': k->r = 1; break;
      default: *p = s; return 0;
    }

    k->typed = 1;
  }
}

static int
sort_option(void* ctx, int ch) {
  struct sort* s = ctx;

  switch(ch) {
    case 'c': s->c = 1; return 0;
    case 'm': return 0;
    case 'u': s->u = 1; return 0;
    case 's': s->stable = 1; return 0;
    case 'o': s->outname = shell_optarg; return 0;
    case 'b': s->glob.bs = s->glob.be = 1; return 0;
    case 'd': s->glob.d = 1; return 0;
    case 'f': s->glob.f = 1; return 0;
    case 'i': s->glob.i = 1; return 0;
    case 'n': s->glob.n = 1; return 0;
    case 'r': s->glob.r = 1; return 0;

    case 't':
      if(!shell_optarg[0] || shell_optarg[1]) {
        s->in.err_arg = shell_optarg;
        s->in.err_msg = "multi-character tab";
        return -1;
      }

      s->tsep = (unsigned char)shell_optarg[0];
      s->havet = 1;
      return 0;

    case 'k': {
      struct sortkey k;
      const char* p = shell_optarg;

      byte_zero(&k, sizeof(k));

      if(sort_keypart(&p, &k.fs, &k.cs, &k, 0) == -1)
        goto bad;

      if(*p == ',') {
        p++;
        k.has_end = 1;

        if(sort_keypart(&p, &k.fe, &k.ce, &k, 1) == -1)
          goto bad;
      }

      if(*p)
        goto bad;

      if(s->nkeys == s->akeys) {
        size_t a = s->akeys ? s->akeys * 2 : 8;
        struct sortkey* n = alloc_re(s->keys, a * sizeof(*n));

        if(!n)
          return -1;

        s->keys = n;
        s->akeys = a;
      }

      s->keys[s->nkeys++] = k;
      return 0;

    bad:
      s->in.err_arg = shell_optarg;
      s->in.err_msg = "invalid key definition";
      return -1;
    }
  }

  return -1;
}

/* -o names a file that may also be an input: the output is written after all
 * input has been read, and a chain cannot do that */
static int
sort_setup(void* ctx) {
  struct sort* s = ctx;

  if(!s->havet)
    s->tsep = -1;

  if(s->c && s->in.files && s->in.files[0] && s->in.files[1]) {
    s->in.err_arg = s->in.files[1];
    s->in.err_msg = "extra operand";
    return -1;
  }

  return s->outname ? 1 : 0;
}

/* start and end of field f of line [s, s+n): blank-separated fields carry their
 * leading blanks, -t fields end before the separator. 0 when the line has no
 * such field */
static int
sort_field(const char* s, size_t n, int tsep, unsigned long f, size_t* start, size_t* end) {
  size_t pos = 0, e;
  unsigned long k;

  for(k = 1;; k++) {
    e = pos;

    if(tsep < 0) {
      while(e < n && sort_blank((unsigned char)s[e]))
        e++;

      while(e < n && !sort_blank((unsigned char)s[e]))
        e++;
    } else {
      while(e < n && (unsigned char)s[e] != tsep)
        e++;
    }

    if(k == f) {
      if(tsep < 0 && pos >= n && k > 1)
        return 0;

      *start = pos;
      *end = e;
      return 1;
    }

    if(e >= n)
      return 0;

    pos = tsep < 0 ? e : e + 1;
  }
}

/* the part of a line a key looks at */
static void
sort_extract(const struct sort* s, const struct sortkey* k, int bs, int be, const char* line, size_t n, const char** a, size_t* an) {
  size_t fst, fen, st, en;

  if(!sort_field(line, n, s->tsep, k->fs, &fst, &fen)) {
    *a = line;
    *an = 0;
    return;
  }

  st = fst;

  if(bs)
    while(st < fen && sort_blank((unsigned char)line[st]))
      st++;

  st += k->cs ? k->cs - 1 : 0;

  if(st > fen)
    st = fen;

  if(!k->has_end) {
    en = n;
  } else {
    size_t efs, efe;

    if(!sort_field(line, n, s->tsep, k->fe, &efs, &efe)) {
      en = n;
    } else if(k->ce) {
      size_t p = efs;

      if(be)
        while(p < efe && sort_blank((unsigned char)line[p]))
          p++;

      p += k->ce;
      en = p < efe ? p : efe;
    } else {
      en = efe;
    }
  }

  if(en < st)
    en = st;

  *a = line + st;
  *an = en - st;
}

/* next significant character of [*p, e) under -d -f -i; -1 at the end */
static int
sort_next(const char** p, const char* e, const struct sortkey* k) {
  while(*p < e) {
    int c = (unsigned char)*(*p)++;

    if(k->d && !(sort_blank(c) || (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')))
      continue;

    if(k->i && (c < 32 || c == 127))
      continue;

    if(k->f && c >= 'a' && c <= 'z')
      c -= 32;

    return c;
  }

  return -1;
}

/* the numeric prefix: sign, digits without leading zeros, fraction digits */
static void
sort_number(const char* s, size_t n, int* neg, const char** ip, size_t* il, const char** fp, size_t* fl) {
  size_t i = 0, st;

  while(i < n && sort_blank((unsigned char)s[i]))
    i++;

  *neg = i < n && s[i] == '-';

  if(*neg)
    i++;

  while(i < n && s[i] == '0')
    i++;

  st = i;

  while(i < n && s[i] >= '0' && s[i] <= '9')
    i++;

  *ip = s + st;
  *il = i - st;
  *fp = s + i;
  *fl = 0;

  if(i < n && s[i] == '.') {
    *fp = s + ++i;

    while(i + *fl < n && s[i + *fl] >= '0' && s[i + *fl] <= '9')
      (*fl)++;

    while(*fl && (*fp)[*fl - 1] == '0')
      (*fl)--;
  }
}

static int
sort_cmp_num(const char* a, size_t an, const char* b, size_t bn) {
  int na, nb, r;
  const char *ia, *fa, *ib, *fb;
  size_t ial, fal, ibl, fbl, i;

  sort_number(a, an, &na, &ia, &ial, &fa, &fal);
  sort_number(b, bn, &nb, &ib, &ibl, &fb, &fbl);

  /* a zero has no sign */
  if(!ial && !fal)
    na = 0;

  if(!ibl && !fbl)
    nb = 0;

  if(na != nb)
    return na ? -1 : 1;

  if(ial != ibl) {
    r = ial < ibl ? -1 : 1;
  } else {
    r = 0;

    for(i = 0; !r && i < ial; i++)
      r = (unsigned char)ia[i] - (unsigned char)ib[i];

    for(i = 0; !r && i < fal && i < fbl; i++)
      r = (unsigned char)fa[i] - (unsigned char)fb[i];

    if(!r && fal != fbl)
      r = fal < fbl ? -1 : 1;
  }

  return na ? -r : r;
}

static int
sort_cmp_text(const char* a, size_t an, const char* b, size_t bn, const struct sortkey* k) {
  const char *pa = a, *pb = b, *ea = a + an, *eb = b + bn;

  for(;;) {
    int ca = sort_next(&pa, ea, k), cb = sort_next(&pb, eb, k);

    if(ca != cb)
      return ca < cb ? -1 : 1;

    if(ca < 0)
      return 0;
  }
}

/* the keys, in command line order; 0 when they are all equal */
static int
sort_cmp_keys(const struct sort* s, const char* a, size_t an, const char* b, size_t bn) {
  size_t i, nk = s->nkeys ? s->nkeys : 1;

  for(i = 0; i < nk; i++) {
    struct sortkey whole, eff;
    const struct sortkey* k = s->nkeys ? &s->keys[i] : &whole;
    const char *ka, *kb;
    size_t kal, kbl;
    int r;

    if(!s->nkeys) {
      byte_zero(&whole, sizeof(whole));
      whole.fs = 1;
    }

    /* a key with modifiers ignores the global options */
    eff = k->typed ? *k : s->glob;
    eff.fs = k->fs;
    eff.cs = k->cs;
    eff.fe = k->fe;
    eff.ce = k->ce;
    eff.has_end = k->has_end;

    if(!s->nkeys) {
      ka = a;
      kal = an;
      kb = b;
      kbl = bn;

      if(eff.bs) {
        while(kal && sort_blank((unsigned char)*ka))
          ka++, kal--;

        while(kbl && sort_blank((unsigned char)*kb))
          kb++, kbl--;
      }
    } else {
      sort_extract(s, &eff, eff.bs, eff.be, a, an, &ka, &kal);
      sort_extract(s, &eff, eff.bs, eff.be, b, bn, &kb, &kbl);
    }

    r = eff.n ? sort_cmp_num(ka, kal, kb, kbl) : sort_cmp_text(ka, kal, kb, kbl, &eff);

    if(r)
      return eff.r ? -r : r;
  }

  return 0;
}

/* keys first; then, unless -u, the whole line (reversed with -r) */
static int
sort_cmp(const struct sort* s, const char* a, size_t an, const char* b, size_t bn) {
  size_t n = an < bn ? an : bn;
  int r = sort_cmp_keys(s, a, an, b, bn);

  if(r || s->u || s->stable)
    return r;

  r = byte_diff(a, n, b);

  if(!r && an != bn)
    r = an < bn ? -1 : 1;

  return s->glob.r ? -r : r;
}

static int
sort_cmp_lines(const struct sort* s, const struct sortline* x, const struct sortline* y) {
  return sort_cmp(s, s->data.s + x->off, x->len, s->data.s + y->off, y->len);
}

/* stable merge sort of a[0..n) */
static void
sort_merge(const struct sort* s, struct sortline* a, struct sortline* t, size_t n) {
  size_t h = n / 2, i = 0, j = h, k = 0;

  if(n < 2)
    return;

  sort_merge(s, a, t, h);
  sort_merge(s, a + h, t, n - h);

  while(i < h && j < n)
    t[k++] = sort_cmp_lines(s, &a[j], &a[i]) < 0 ? a[j++] : a[i++];

  while(i < h)
    t[k++] = a[i++];

  while(j < n)
    t[k++] = a[j++];

  byte_copy(a, k * sizeof(*a), t);
}

/* reads every line, sorts them */
static int
sort_load(struct sort* s) {
  const char* line;
  int had_nl;
  ssize_t r;

  while((r = filter_in_line(&s->in, &line, &had_nl)) >= 0) {
    if(s->nlines == s->alines) {
      size_t a = s->alines ? s->alines * 2 : 1024;
      struct sortline* n = alloc_re(s->lines, a * sizeof(*n));

      if(!n)
        return -1;

      s->lines = n;
      s->alines = a;
    }

    s->lines[s->nlines].off = s->data.len;
    s->lines[s->nlines].len = (size_t)r;
    s->nlines++;
    stralloc_catb(&s->data, line, (size_t)r);
    stralloc_catc(&s->data, '\n');
  }

  if(s->nlines && !(s->tmp = alloc(s->nlines * sizeof(*s->tmp))))
    return -1;

  sort_merge(s, s->lines, s->tmp, s->nlines);
  return 0;
}

/* the next batch of sorted lines (at most a block), dropping duplicates with -u */
static int
sort_batch(struct sort* s, const char** unit, size_t* len) {
  s->out.len = 0;

  while(s->next < s->nlines && s->out.len < 65536) {
    struct sortline* l = &s->lines[s->next];

    if(s->u && s->next > 0 && sort_cmp_keys(s, s->data.s + s->lines[s->next - 1].off, s->lines[s->next - 1].len,
                                            s->data.s + l->off, l->len) == 0) {
      s->next++;
      continue;
    }

    stralloc_catb(&s->out, s->data.s + l->off, l->len + 1);
    s->next++;
  }

  *unit = s->out.s;
  *len = s->out.len;
  return s->out.len > 0;
}

/* -c: compare each line with the one before it, nothing is kept */
static void
sort_check(struct sort* s) {
  const char* line;
  int had_nl;
  ssize_t r;
  int have = 0;

  while((r = filter_in_line(&s->in, &line, &had_nl)) >= 0) {
    s->lineno++;

    if(have) {
      int c = sort_cmp_keys(s, s->prev.s, s->prev.len, line, (size_t)r);

      if(c > 0 || (c == 0 && s->u)) {
        builtin_errmsg(s->in.errargv, (char*)filter_in_name(&s->in), "disorder");
        s->disorder = 1;
        return;
      }
    }

    s->prev.len = 0;
    stralloc_catb(&s->prev, line, (size_t)r);
    have = 1;
  }
}

/* -o: the sorted lines go to the file once all input is read */
static void
sort_to_file(struct sort* s) {
  int fd = open_trunc(s->outname);
  buffer ob;
  char obuf[4096];
  const char* unit;
  size_t len;

  if(fd == -1) {
    builtin_error(s->in.errargv, (char*)s->outname);
    s->in.had_error = 1;
    return;
  }

  buffer_init(&ob, &buffer_op_write, fd, obuf, sizeof(obuf));

  while(sort_batch(s, &unit, &len))
    buffer_put(&ob, unit, len);

  if(buffer_flush(&ob) < 0)
    s->in.had_error = 1;

  close(fd);
}

static int
sort_step(void* arg, const char** unit, size_t* len) {
  struct sort* s = arg;

  if(!s->loaded) {
    s->loaded = 1;

    if(s->c) {
      sort_check(s);
      return 0;
    }

    if(sort_load(s) < 0) {
      s->in.had_error = 1;
      return 0;
    }

    if(s->outname) {
      sort_to_file(s);
      return 0;
    }
  }

  if(s->c)
    return 0;

  return sort_batch(s, unit, len);
}

static int
sort_status(void* ctx) {
  struct sort* s = ctx;

  if(s->in.had_error)
    return 2;

  return s->disorder;
}

static void
sort_finish(void* ctx) {
  struct sort* s = ctx;

  stralloc_free(&s->data);
  stralloc_free(&s->out);
  stralloc_free(&s->prev);
  alloc_free(s->lines);
  alloc_free(s->tmp);
  alloc_free(s->keys);
}

const struct filter_ops sort_ops = {
    .opts = "cmo:bdfinrust:k:",
    .size = sizeof(struct sort),
    .option = sort_option,
    .setup = sort_setup,
    .step = sort_step,
    .status = sort_status,
    .finish = sort_finish,
    .err_status = 2,
};

FILTER_BUILTIN(sort)
#endif /* BUILTIN_SORT */
