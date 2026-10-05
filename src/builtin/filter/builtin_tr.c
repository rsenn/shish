#include "builtin_config.h"

#if BUILTIN_TR

#include "../../builtin.h"
#include "../../fdtable.h"
#include "../../sh.h"
#include "../../../lib/shell.h"
#include "../../../lib/byteset.h"
#include "../../../lib/byte.h"
#include "../../../lib/str.h"
#include "../../../lib/alloc.h"
#include "../../../lib/utf8.h"
#include "../../../lib/stralloc.h"

const char help_tr[] = "    Translate, squeeze or delete characters (UTF-8 when the locale variables\n"
                       "    name it, else bytes).\n"
                       "\n"
                       "    -c, -C          use the complement of string1\n"
                       "    -d              delete characters in string1\n"
                       "    -s              squeeze runs of a repeated character in the last string to one\n"
                       "    string1         a list: a-z ranges, [:class:], [=c=], [c*n], \\n \\ooo escapes\n"
                       "    string2         what string1 translates to (its last character is repeated)\n";

/* a growable list of code points */
struct cpv {
  unsigned* v;
  size_t n, cap;
};

/* a code point >= 256 of string1 and what it translates to (k: its position, so the last of equals wins) */
struct ent {
  unsigned from, to, k;
};

struct tr {
  struct filter_in in;
  unsigned map[256];          /* translation of code points < 256; identity when not translating */
  unsigned char del[BYTESET_SIZE], sq[BYTESET_SIZE];
  struct ent *hi, *sqhi;      /* code points >= 256: string1 (sorted), squeeze set (sorted) */
  size_t nhi, nsqhi;
  unsigned last2;             /* string2's last code point, what -c maps to */
  const char *s1, *s2;
  unsigned c : 1, d : 1, s : 1, utf8 : 1, tr2 : 1, sqcomp : 1;
  long last;                  /* the last code point written while squeezing, or -1 */
  char carry[4];              /* a character cut off by the end of the window */
  size_t ncarry;
  stralloc out, buf;
};

static int
tr_option(void* ctx, int ch) {
  struct tr* t = ctx;

  switch(ch) {
    case 'c': case 'C': t->c = 1; return 0;
    case 'd': t->d = 1; return 0;
    case 's': t->s = 1; return 0;
  }

  return -1;
}

static int
cpv_add(struct cpv* l, unsigned c) {
  if(l->n == l->cap) {
    unsigned* p = alloc_re(l->v, (l->cap ? l->cap * 2 : 64) * sizeof(unsigned));

    if(!p)
      return -1;

    l->v = p;
    l->cap = l->cap ? l->cap * 2 : 64;
  }

  l->v[l->n++] = c;
  return 0;
}

/* one possibly escaped character at *sp, which moves past it: "\n" "\101" "x" "é" */
static unsigned
tr_atom(const char** sp, int utf8) {
  const char* s = *sp;
  unsigned c = (unsigned char)*s++;

  if(c == '\\' && *s) {
    const char* e = "a\ab\bf\fn\nr\rt\tv\v";
    int k;

    if(*s >= '0' && *s <= '7') {
      for(c = 0, k = 0; k < 3 && *s >= '0' && *s <= '7'; k++)
        c = c * 8 + *s++ - '0';

      c &= 255;
    } else {
      size_t l = utf8 ? text_charlen(1, s, str_len(s)) : 1;

      for(c = (unsigned char)*s; *e && *e != *s; e += 2) {}

      if(*e && l == 1)
        c = (unsigned char)e[1];
      else if(l > 1)
        u8decode(s, l, &c);

      s += l;
    }
  } else if(utf8 && c >= 0x80) {
    int l = u8decode(s - 1, str_len(s - 1), &c);

    if(l > 1)
      s += l - 1;
    else
      c = (unsigned char)s[-1];
  }

  *sp = s;
  return c;
}

/* expands a string into its code points; a trailing "[c*]" is left for the caller:
 *   "a-cx"  -> "abcx"        "[:digit:]" -> "0123456789"
 *   "[x*3]" -> "xxx"         "[y*]"      -> *fill = 'y', out gets nothing there
 * returns 0, or -1 with *err set */
static int
tr_expand(const char* s, struct cpv* out, long* fill, size_t* fillpos, const char** err, int utf8) {
  *fill = -1;

  while(*s) {
    unsigned c;

    if(s[0] == '[' && s[1] == ':') {
      const char* e = s + 2;
      unsigned char set[BYTESET_SIZE];
      int b;

      while(*e && !(e[0] == ':' && e[1] == ']'))
        e++;

      if(!*e || (byteset_zero(set), !byteset_class(set, s + 2, (size_t)(e - s - 2)))) {
        *err = "invalid character class";
        return -1;
      }

      for(b = byteset_next(set, 0); b >= 0; b = byteset_next(set, b + 1))
        cpv_add(out, (unsigned)b);

      s = e + 2;
      continue;
    }

    if(s[0] == '[' && s[1] == '=' && s[2] && s[3] == '=' && s[4] == ']') {
      cpv_add(out, (unsigned char)s[2]);
      s += 5;
      continue;
    }

    if(s[0] == '[' && s[1]) {
      const char* p = s + 1;

      c = tr_atom(&p, utf8);

      if(*p == '*') {
        const char* d = ++p;
        unsigned long n = 0;
        int base = *d == '0' ? 8 : 10;

        while(*d >= '0' && *d < '0' + base)
          n = n * base + *d++ - '0';

        if(*d == ']') {
          if(d == p) {
            *fill = c;
            *fillpos = out->n;
          } else
            while(n--)
              cpv_add(out, c);

          s = d + 1;
          continue;
        }
      }
    }

    c = tr_atom(&s, utf8);

    if(*s == '-' && s[1]) {
      unsigned hi;

      s++;
      hi = tr_atom(&s, utf8);

      if(hi < c) {
        *err = "range-endpoints are in reverse collating sequence order";
        return -1;
      }

      while(c <= hi)
        cpv_add(out, c++);
    } else
      cpv_add(out, c);
  }

  return 0;
}

static int
tr_fail(struct tr* t, const char* arg, const char* msg) {
  t->in.err_arg = arg;
  t->in.err_msg = msg;
  return -1;
}

/* orders by (from, k): shell sort */
static void
ent_sort(struct ent* a, size_t n) {
  size_t gap, i, j;

  for(gap = n / 2; gap; gap /= 2)
    for(i = gap; i < n; i++) {
      struct ent e = a[i];

      for(j = i; j >= gap && (a[j - gap].from > e.from || (a[j - gap].from == e.from && a[j - gap].k > e.k)); j -= gap)
        a[j] = a[j - gap];

      a[j] = e;
    }
}

/* after ent_sort: one entry per from, the last one; returns the new count */
static size_t
ent_uniq(struct ent* a, size_t n) {
  size_t i, m = 0;

  for(i = 0; i < n; i++)
    if(i + 1 == n || a[i + 1].from != a[i].from)
      a[m++] = a[i];

  return m;
}

static const struct ent*
ent_find(const struct ent* a, size_t n, unsigned cp) {
  size_t lo = 0, hi = n;

  while(lo < hi) {
    size_t mid = (lo + hi) / 2;

    if(a[mid].from == cp)
      return &a[mid];

    if(a[mid].from < cp)
      lo = mid + 1;
    else
      hi = mid;
  }

  return NULL;
}

/* the entries of l at or above 256, sorted; k and to as given by the callers */
static struct ent*
ent_high(const struct cpv* l, const struct cpv* to, size_t* np) {
  struct ent* a = alloc((l->n + 1) * sizeof(*a));
  size_t i, n = 0;

  if(!a)
    return NULL;

  for(i = 0; i < l->n; i++)
    if(l->v[i] >= 256) {
      a[n].from = l->v[i];
      a[n].to = to && to->n ? to->v[i < to->n ? i : to->n - 1] : 0;
      a[n].k = (unsigned)i;
      n++;
    }

  ent_sort(a, n);
  *np = ent_uniq(a, n);
  return a;
}

static int
tr_setup(void* ctx) {
  struct tr* t = ctx;
  struct cpv l1 = {0}, l2 = {0}, comp = {0}, *m1 = &l1;
  const char* err = NULL;
  char** a = t->in.files;
  long fill;
  int ret = -1;
  size_t fpos = 0, k;
  unsigned i;

  t->last = -1;
  t->utf8 = sh_utf8();
  t->s1 = a ? a[0] : NULL;
  t->s2 = t->s1 ? a[1] : NULL;
  t->in.files = NULL;

  if(!t->s1)
    return tr_fail(t, "", "missing operand");

  if(t->s2 && a[2])
    return tr_fail(t, a[2], "extra operand");

  if(!t->d && !t->s && !t->s2)
    return tr_fail(t, t->s1, "missing operand after string1");

  if(t->d && t->s2 && !t->s)
    return tr_fail(t, t->s2, "extra operand");

  if(tr_expand(t->s1, &l1, &fill, &fpos, &err, t->utf8) < 0 ||
     (t->s2 && tr_expand(t->s2, &l2, &fill, &fpos, &err, t->utf8) < 0)) {
    tr_fail(t, "", err);
    goto out;
  }

  /* the complement of string1 below 256, ascending; above it -c is decided per character */
  if(t->c) {
    unsigned char set[BYTESET_SIZE];
    int b;

    byteset_zero(set);

    for(k = 0; k < l1.n; k++)
      if(l1.v[k] < 256)
        byteset_add(set, l1.v[k]);

    byteset_invert(set);

    for(b = byteset_next(set, 0); b >= 0; b = byteset_next(set, b + 1))
      cpv_add(&comp, (unsigned)b);

    m1 = &comp;
  }

  for(i = 0; i < 256; i++)
    t->map[i] = i;

  /* "[c*]" pads string2 out to string1's length */
  if(t->s2 && fill >= 0 && l2.n < m1->n + (t->c ? 0 : 0) && l2.n < l1.n + (t->c ? comp.n : 0)) {
    size_t n = (t->c ? comp.n : l1.n) - l2.n, j;

    for(j = 0; j < n; j++)
      cpv_add(&l2, (unsigned)fill);

    for(j = l2.n - n; j-- > fpos;)
      l2.v[j + n] = l2.v[j];

    for(j = 0; j < n; j++)
      l2.v[fpos + j] = (unsigned)fill;
  }

  t->tr2 = t->s2 && !t->d;

  if(t->tr2 && !l2.n) {
    tr_fail(t, "", "string2 must not be empty");
    goto out;
  }

  t->last2 = l2.n ? l2.v[l2.n - 1] : 0;
  t->sqcomp = t->c && !t->s2;

  for(k = 0; k < m1->n; k++) {
    unsigned f = m1->v[k];

    if(f >= 256)
      continue;

    if(t->d)
      byteset_add(t->del, f);
    else if(t->tr2)
      t->map[f] = l2.v[k < l2.n ? k : l2.n - 1];
  }

  if(t->s) {
    struct cpv* sq = t->s2 ? &l2 : m1;

    for(k = 0; k < sq->n; k++)
      if(sq->v[k] < 256)
        byteset_add(t->sq, sq->v[k]);
  }

  if(t->utf8) {
    if(!(t->hi = ent_high(&l1, t->tr2 && !t->c ? &l2 : NULL, &t->nhi)))
      goto out;

    if(t->s && !t->sqcomp && !(t->sqhi = ent_high(t->s2 ? &l2 : &l1, NULL, &t->nsqhi)))
      goto out;
  }

  ret = 0;
out:
  alloc_free(l1.v);
  alloc_free(l2.v);
  alloc_free(comp.v);
  return ret;
}

/* one code point through the sets: 1 and *cp translated, or 0 when deleted */
static int
tr_char(struct tr* t, unsigned* cp) {
  unsigned c = *cp;

  if(c < 256) {
    if(t->d && byteset_has(t->del, c))
      return 0;

    c = t->map[c];
  } else {
    const struct ent* e = ent_find(t->hi, t->nhi, c);
    int match = t->c ? !e : e != NULL;

    if(match && t->d)
      return 0;

    if(match && t->tr2)
      c = t->c ? t->last2 : e->to;
  }

  *cp = c;
  return 1;
}

/* does the squeeze set hold c (already translated)? */
static int
tr_squeeze(struct tr* t, unsigned c) {
  if(c < 256)
    return byteset_has(t->sq, c);

  if(t->sqcomp)
    return !ent_find(t->hi, t->nhi, c);

  return ent_find(t->sqhi, t->nsqhi, c) != NULL;
}

/* one code point out, squeezing runs */
static void
tr_put(struct tr* t, unsigned c) {
  char b[4];

  if(t->s && tr_squeeze(t, c)) {
    if(t->last == (long)c)
      return;

    t->last = c;
  } else
    t->last = -1;

  if(t->utf8 && c >= 0x80)
    stralloc_catb(&t->out, b, (size_t)u8encode(b, c));
  else
    stralloc_catc(&t->out, (char)c);
}

static int
tr_step(void* arg, const char** unit, size_t* len) {
  struct tr* t = arg;
  const char* p;
  ssize_t n;
  size_t i = 0;

  t->out.len = 0;

  if((n = filter_in_peek(&t->in, &p)) <= 0) {
    /* end of input: a character left cut off goes out as it is */
    if(!t->ncarry)
      return 0;

    stralloc_catb(&t->out, t->carry, t->ncarry);
    t->ncarry = 0;
    *unit = t->out.s;
    *len = t->out.len;
    return 1;
  }

  if(n > 65536)
    n = 65536;

  if(t->ncarry) {
    t->buf.len = 0;
    stralloc_catb(&t->buf, t->carry, t->ncarry);
    stralloc_catb(&t->buf, p, (size_t)n);
    p = t->buf.s;
    t->ncarry = 0;
    filter_in_skip(&t->in, (size_t)n);
    n = t->buf.len;
  } else
    filter_in_skip(&t->in, (size_t)n);

  stralloc_ready(&t->out, (size_t)n);

  while(i < (size_t)n) {
    unsigned c = (unsigned char)p[i];
    int l = 1;

    if(t->utf8 && c >= 0x80) {
      l = u8decode(p + i, (size_t)n - i, &c);

      if(l == -2) {
        t->ncarry = (size_t)n - i;
        byte_copy(t->carry, t->ncarry, p + i);
        break;
      }

      if(l < 0) {
        /* not UTF-8: the byte goes out untouched */
        stralloc_catc(&t->out, p[i++]);
        t->last = -1;
        continue;
      }
    }

    i += (size_t)l;

    if(tr_char(t, &c))
      tr_put(t, c);
  }

  *unit = t->out.s;
  *len = t->out.len;
  return 1;
}

static int
tr_status(void* ctx) {
  return ((struct tr*)ctx)->in.had_error;
}

static void
tr_finish(void* ctx) {
  struct tr* t = ctx;

  alloc_free(t->hi);
  alloc_free(t->sqhi);
  stralloc_free(&t->out);
  stralloc_free(&t->buf);
}

const struct filter_ops tr_ops = {
    .opts = "cCds",
    .size = sizeof(struct tr),
    .option = tr_option,
    .setup = tr_setup,
    .step = tr_step,
    .status = tr_status,
    .finish = tr_finish,
};

FILTER_BUILTIN(tr)
#endif /* BUILTIN_TR */
