#include "../../builtin.h"
#include "../../fdtable.h"
#include "../../../lib/shell.h"
#include "../../../lib/byteset.h"
#include "../../../lib/str.h"
#include "../../../lib/stralloc.h"

const char help_tr[] = "    Translate, squeeze or delete characters (bytes, not UTF-8 characters).\n"
                       "\n"
                       "    -c, -C          use the complement of string1\n"
                       "    -d              delete characters in string1\n"
                       "    -s              squeeze runs of a repeated character in the last string to one\n"
                       "    string1         a list: a-z ranges, [:class:], [=c=], [c*n], \\n \\ooo escapes\n"
                       "    string2         what string1 translates to (its last character is repeated)\n";

struct tr {
  struct filter_in in;
  unsigned char map[256];     /* translation; identity when not translating */
  unsigned char del[BYTESET_SIZE], sq[BYTESET_SIZE];
  const char *s1, *s2;
  unsigned c : 1, d : 1, s : 1;
  int last;                   /* the last byte written while squeezing, or -1 */
  stralloc out;
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

/* one possibly escaped byte at *sp, which moves past it: "\n" "\101" "x" */
static int
tr_atom(const char** sp) {
  const char* s = *sp;
  int c = (unsigned char)*s++;

  if(c == '\\' && *s) {
    const char* e = "a\ab\bf\fn\nr\rt\tv\v";
    int k;

    if(*s >= '0' && *s <= '7') {
      for(c = 0, k = 0; k < 3 && *s >= '0' && *s <= '7'; k++)
        c = c * 8 + *s++ - '0';

      c &= 255;
    } else {
      for(c = (unsigned char)*s; *e && *e != *s; e += 2) {}

      if(*e)
        c = (unsigned char)e[1];

      s++;
    }
  }

  *sp = s;
  return c;
}

/* expands a string into its bytes; a trailing "[c*]" is left for the caller:
 *   "a-cx"  -> "abcx"        "[:digit:]" -> "0123456789"
 *   "[x*3]" -> "xxx"         "[y*]"      -> *fill = 'y', out gets nothing there
 * returns 0, or -1 with *err set */
static int
tr_expand(const char* s, stralloc* out, int* fill, size_t* fillpos, const char** err) {
  *fill = -1;

  while(*s) {
    int c;

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
        stralloc_catc(out, (char)b);

      s = e + 2;
      continue;
    }

    if(s[0] == '[' && s[1] == '=' && s[2] && s[3] == '=' && s[4] == ']') {
      stralloc_catc(out, s[2]);
      s += 5;
      continue;
    }

    if(s[0] == '[' && s[1]) {
      const char* p = s + 1;

      c = tr_atom(&p);

      if(*p == '*') {
        const char* d = ++p;
        unsigned long n = 0;
        int base = *d == '0' ? 8 : 10;

        while(*d >= '0' && *d < '0' + base)
          n = n * base + *d++ - '0';

        if(*d == ']') {
          if(d == p) {
            *fill = c;
            *fillpos = out->len;
          } else
            while(n--)
              stralloc_catc(out, (char)c);

          s = d + 1;
          continue;
        }
      }
    }

    c = tr_atom(&s);

    if(*s == '-' && s[1]) {
      int hi;

      s++;
      hi = tr_atom(&s);

      if(hi < c) {
        *err = "range-endpoints are in reverse collating sequence order";
        return -1;
      }

      while(c <= hi)
        stralloc_catc(out, (char)c++);
    } else
      stralloc_catc(out, (char)c);
  }

  return 0;
}

static int
tr_fail(struct tr* t, const char* arg, const char* msg) {
  t->in.err_arg = arg;
  t->in.err_msg = msg;
  return -1;
}

static void
tr_set(unsigned char* set, const stralloc* l) {
  size_t i;

  for(i = 0; i < l->len; i++)
    byteset_add(set, (unsigned char)l->s[i]);
}

static int
tr_setup(void* ctx) {
  struct tr* t = ctx;
  stralloc l1 = {0}, l2 = {0};
  const char* err = NULL;
  char** a = t->in.files;
  int fill, i, ret = -1;
  size_t fpos = 0;

  t->last = -1;
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

  if(tr_expand(t->s1, &l1, &fill, &fpos, &err) < 0 || (t->s2 && tr_expand(t->s2, &l2, &fill, &fpos, &err) < 0)) {
    tr_fail(t, "", err);
    goto out;
  }

  if(t->c) {
    unsigned char set[BYTESET_SIZE];
    int b;

    byteset_zero(set);
    tr_set(set, &l1);
    byteset_invert(set);
    l1.len = 0;

    for(b = byteset_next(set, 0); b >= 0; b = byteset_next(set, b + 1))
      stralloc_catc(&l1, (char)b);
  }

  for(i = 0; i < 256; i++)
    t->map[i] = (unsigned char)i;

  /* "[c*]" pads string2 out to string1's length */
  if(t->s2 && fill >= 0 && l2.len < l1.len) {
    size_t k, n = l1.len - l2.len;

    stralloc_readyplus(&l2, n);
    for(k = l2.len; k-- > fpos;)
      l2.s[k + n] = l2.s[k];
    for(k = 0; k < n; k++)
      l2.s[fpos + k] = (char)fill;
    l2.len += n;
  }

  if(t->d)
    tr_set(t->del, &l1);
  else if(t->s2) {
    size_t k;

    if(!l2.len)
      return tr_fail(t, "", "string2 must not be empty"), ret;

    for(k = 0; k < l1.len; k++)
      t->map[(unsigned char)l1.s[k]] = (unsigned char)l2.s[k < l2.len ? k : l2.len - 1];
  }

  if(t->s)
    tr_set(t->sq, t->s2 ? &l2 : &l1);

  ret = 0;
out:
  stralloc_free(&l1);
  stralloc_free(&l2);
  return ret;
}

static int
tr_step(void* arg, const char** unit, size_t* len) {
  struct tr* t = arg;
  const char* p;
  ssize_t n;
  size_t i;

  if((n = filter_in_peek(&t->in, &p)) <= 0)
    return 0;

  if(n > 65536)
    n = 65536;

  t->out.len = 0;
  stralloc_ready(&t->out, (size_t)n);

  for(i = 0; i < (size_t)n; i++) {
    unsigned char c = (unsigned char)p[i];

    if(t->d && byteset_has(t->del, c))
      continue;

    c = t->map[c];

    if(t->s && byteset_has(t->sq, c)) {
      if(t->last == c)
        continue;

      t->last = c;
    } else
      t->last = -1;

    t->out.s[t->out.len++] = (char)c;
  }

  filter_in_skip(&t->in, (size_t)n);
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
  stralloc_free(&((struct tr*)ctx)->out);
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
