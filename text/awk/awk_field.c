/* records ($0) and fields ($1..$NF): storage, lazy split, lazy $0
 * rebuild, and the shared splitter also used by split(). */
#include "awk_internal.h"
#include "../../lib/str.h"
#include "../../lib/byte.h"
#include "../../lib/alloc.h"
#include "../../lib/scan.h"

void
awk_rec_init(struct awk_rec* r) {
  byte_zero(r, sizeof(*r));
  stralloc_init(&r->line);
}

void
awk_rec_free(struct awk_rec* r) {
  size_t i;

  stralloc_free(&r->line);

  for(i = 0; i < r->cap; i++)
    awk_cell_free(&r->f[i]);

  alloc_free(r->f);
  byte_zero(r, sizeof(*r));
}

static void
rec_ensure_cap(struct awk_rec* r, size_t n) {
  if(n <= r->cap)
    return;

  {
    size_t newcap = r->cap ? r->cap : 8;

    while(newcap < n)
      newcap *= 2;

    r->f = alloc_re(r->f, newcap * sizeof(awk_cell));
    byte_zero(&r->f[r->cap], (newcap - r->cap) * sizeof(awk_cell));
    r->cap = newcap;
  }
}

void
awk_rec_setline(struct awk_state* st, const char* s, size_t n) {
  stralloc_copyb(&st->rec.line, s, n);
  st->rec.split_done = 0;
  st->rec.dirty = 0;
  st->rec.nf = 0;
}

size_t
awk_split(struct awk_state* st,
          const char* s,
          size_t n,
          const char* fs,
          size_t fslen,
          struct awk_span** out,
          size_t* outcap) {
  size_t nout = 0;

#define PUSH(sp, ln) \
  do { \
    if(nout == *outcap) { \
      *outcap = *outcap ? *outcap * 2 : 8; \
      *out = alloc_re(*out, *outcap * sizeof(struct awk_span)); \
    } \
    (*out)[nout].s = (sp); \
    (*out)[nout].len = (ln); \
    nout++; \
  } while(0)

  if(fslen == 0) {
    /* GNU extension: null FS splits into individual bytes */
    size_t i;

    for(i = 0; i < n; i++)
      PUSH(s + i, 1);
  } else if(fslen == 1 && fs[0] == ' ') {
    /* default: runs of <blank>/newline, leading/trailing trimmed */
    size_t i = 0;

    while(i < n) {
      size_t start;

      while(i < n && (s[i] == ' ' || s[i] == '\t' || s[i] == '\n'))
        i++;

      if(i >= n)
        break;

      start = i;

      while(i < n && s[i] != ' ' && s[i] != '\t' && s[i] != '\n')
        i++;

      PUSH(s + start, i - start);
    }
  } else if(fslen == 1) {
    /* a single non-space character: split on each literal occurrence,
       empty fields allowed */
    size_t start = 0, i;

    if(n == 0) {
      /* no fields at all */
    } else {
      for(i = 0; i < n; i++) {
        if(s[i] == fs[0]) {
          PUSH(s + start, i - start);
          start = i + 1;
        }
      }

      PUSH(s + start, n - start);
    }
  } else {
    /* a multi-character FS is an ERE (dfa_search) */
    struct dfa re;

    byte_zero(&re, sizeof(re));

    if(dfa_compile(&re, fs, fslen, DFA_ERE) != DFA_OK) {
      awk_runtime_error(st, "invalid FS regular expression");
      PUSH(s, n);
    } else {
      size_t rn = awk_split_re(st, s, n, &re, out, outcap);

      dfa_free(&re);
      return rn; /* awk_split_re already pushed into *out; nout is unused here */
    }
  }

#undef PUSH
  return nout;
}

size_t
awk_split_re(struct awk_state* st,
             const char* s,
             size_t n,
             struct dfa* re,
             struct awk_span** out,
             size_t* outcap) {
  size_t nout = 0, start = 0, from = 0;
  struct dfa_span m;

  (void)st;

#define PUSH(sp, ln) \
  do { \
    if(nout == *outcap) { \
      *outcap = *outcap ? *outcap * 2 : 8; \
      *out = alloc_re(*out, *outcap * sizeof(struct awk_span)); \
    } \
    (*out)[nout].s = (sp); \
    (*out)[nout].len = (ln); \
    nout++; \
  } while(0)

  while(from <= n && dfa_search(re, s, n, from, &m) == 1) {
    if(m.end == m.start)
      break; /* an empty match can't split anything further */

    PUSH(s + start, m.start - start);
    start = m.end;
    from = m.end;
  }

  PUSH(s + start, n - start);
#undef PUSH
  return nout;
}

void
awk_rec_ensure_split(struct awk_state* st) {
  struct awk_span* spans = NULL;
  size_t spancap = 0, nf, i;
  awk_cell* fs;

  if(st->rec.split_done)
    return;

  fs = awk_global(st, SP_FS);
  nf = awk_split(st,
                 st->rec.line.s ? st->rec.line.s : "",
                 st->rec.line.len,
                 fs->str ? fs->str : " ",
                 fs->str ? str_len(fs->str) : 1,
                 &spans,
                 &spancap);

  rec_ensure_cap(&st->rec, nf + 1);

  for(i = 0; i < nf; i++) {
    double v;
    int numeric = awk_looks_numeric(spans[i].s, spans[i].len, &v);

    awk_cell_free(&st->rec.f[i + 1]);
    awk_cell_set_str(&st->rec.f[i + 1], spans[i].s, spans[i].len, numeric);

    if(numeric)
      st->rec.f[i + 1].num = v;
  }

  for(i = nf; i + 1 < st->rec.cap && st->rec.f[i + 1].type != CELL_UNINIT; i++)
    awk_cell_free(&st->rec.f[i + 1]);

  alloc_free(spans);
  st->rec.nf = nf;
  st->rec.split_done = 1;
  st->rec.dirty = 0;
  awk_cell_set_num(awk_global(st, SP_NF), (double)nf);
}

static void
rec_rebuild(struct awk_state* st) {
  awk_cell* ofs = awk_global(st, SP_OFS);
  size_t i;

  stralloc_zero(&st->rec.line);

  for(i = 1; i <= st->rec.nf; i++) {
    if(i > 1)
      stralloc_cats(&st->rec.line, ofs->str ? ofs->str : " ");

    stralloc_cats(&st->rec.line, awk_tostr(st, &st->rec.f[i], 0));
  }

  st->rec.dirty = 0;
}

awk_cell*
awk_rec_field(struct awk_state* st, long n) {
  if(n < 0) {
    awk_runtime_error(st, "field index is negative");
    n = 0;
  }

  if(n == 0) {
    if(st->rec.dirty)
      rec_rebuild(st);

    rec_ensure_cap(&st->rec, 1);
    awk_cell_free(&st->rec.f[0]);
    awk_cell_set_str(&st->rec.f[0], st->rec.line.s ? st->rec.line.s : "", st->rec.line.len, 0);
    return &st->rec.f[0];
  }

  awk_rec_ensure_split(st);

  if((size_t)n > st->rec.nf) {
    /* reading $(NF+k) for k>0 yields "" without growing NF (POSIX);
       only an *assignment* through awk_lvalue grows it (awk_rec_setnf) */
    rec_ensure_cap(&st->rec, (size_t)n + 1);
    return &st->rec.f[n];
  }

  return &st->rec.f[n];
}

awk_cell*
awk_rec_field_for_write(struct awk_state* st, long n) {
  size_t i;

  if(n < 0) {
    awk_runtime_error(st, "field index is negative");
    n = 0;
  }

  awk_rec_ensure_split(st);
  rec_ensure_cap(&st->rec, (size_t)n + 1);

  for(i = st->rec.nf + 1; i < (size_t)n; i++)
    awk_cell_set_str(&st->rec.f[i], "", 0, 0);

  if((size_t)n > st->rec.nf) {
    st->rec.nf = (size_t)n;
    awk_cell_set_num(awk_global(st, SP_NF), (double)n);
  }

  st->rec.dirty = 1;
  return &st->rec.f[n];
}

void
awk_rec_setnf(struct awk_state* st, long nf) {
  size_t i;

  awk_rec_ensure_split(st);

  if(nf < 0)
    nf = 0;

  rec_ensure_cap(&st->rec, (size_t)nf + 1);

  for(i = (size_t)nf + 1; i <= st->rec.nf; i++)
    awk_cell_free(&st->rec.f[i]);

  for(i = st->rec.nf + 1; i <= (size_t)nf; i++)
    awk_cell_set_str(&st->rec.f[i], "", 0, 0);

  st->rec.nf = (size_t)nf;
  st->rec.dirty = 1;
  awk_cell_set_num(awk_global(st, SP_NF), (double)nf);
}
