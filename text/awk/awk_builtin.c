/* built-in function dispatch (A_CALLBUILTIN). */
#include "awk_internal.h"
#include "../../lib/str.h"
#include "../../lib/byte.h"
#include "../../lib/alloc.h"
#include "../../lib/fmt.h"
#include <math.h>
#include <time.h>

static awk_cell
num_cell(double n) {
  awk_cell c;

  byte_zero(&c, sizeof(c));
  c.type = CELL_NUM;
  c.num = n;
  return c;
}

static awk_cell
str_cell_tmp(struct awk_state* st, const char* s, size_t n) {
  awk_cell c;
  char* buf = arena_alloc(&st->tmp, n + 1, 1);

  byte_zero(&c, sizeof(c));
  c.type = CELL_STR;

  if(buf) {
    byte_copy(buf, n, s);
    buf[n] = 0;
  }

  c.str = buf ? buf : (char*)"";
  return c;
}

static awk_cell
arg_eval(struct awk_state* st, struct anode** ap) {
  awk_cell c;

  byte_zero(&c, sizeof(c));

  if(!*ap)
    return c;

  c = awk_eval(st, *ap);
  *ap = (*ap)->next;
  return c;
}

static awk_cell*
var_cell(struct awk_state* st, struct anode* n) {
  return (n->flags & F_LOCAL) ? st->frame->slot[n->idx] : awk_global(st, n->idx);
}

static int
compile_regex_arg(struct awk_state* st, struct anode* n, struct dfa* scratch, struct dfa** out,
                   int* dynamic) {
  if(n->op == A_REGEX) {
    *out = n->u.re;
    *dynamic = 0;
    return 1;
  }

  {
    awk_cell v = awk_eval(st, n);
    const char* pat = awk_tostr(st, &v, 0);

    byte_zero(scratch, sizeof(*scratch));

    if(dfa_compile(scratch, pat, str_len(pat), DFA_ERE) != DFA_OK) {
      awk_runtime_error(st, "invalid dynamic regular expression");
      return 0;
    }

    *out = scratch;
    *dynamic = 1;
    return 1;
  }
}

static struct awk_lvalue
field0_lvalue(void) {
  struct awk_lvalue lv;

  byte_zero(&lv, sizeof(lv));
  lv.kind = LV_FIELD;
  lv.fieldn = 0;
  return lv;
}

static int
repl_out(void* ctx, const char* s, size_t n) {
  stralloc_catb((stralloc*)ctx, s, n);
  return 0;
}

static awk_cell
do_sub(struct awk_state* st, struct anode* args, int global) {
  struct dfa scratch, *re;
  int dynamic;
  struct dfa_repl* rc;
  awk_cell replv;
  const char* repl;
  struct awk_lvalue lv;
  awk_cell* target;
  const char* subject;
  size_t subjlen;
  stralloc out;
  unsigned nreplaced = 0;

  if(!args || !args->next) {
    awk_runtime_error(st, "sub/gsub: too few arguments");
    return num_cell(0);
  }

  if(!compile_regex_arg(st, args, &scratch, &re, &dynamic))
    return num_cell(0);

  replv = awk_eval(st, args->next);
  repl = awk_tostr(st, &replv, 0);

  if(dfa_repl_compile(&rc, repl, str_len(repl), 0) != DFA_OK) {
    if(dynamic)
      dfa_free(re);

    awk_runtime_error(st, "out of memory");
    return num_cell(0);
  }

  lv = args->next->next ? awk_lvalue(st, args->next->next) : field0_lvalue();
  target = awk_lvalue_get(st, &lv);
  subject = target->str ? target->str : "";
  subjlen = str_len(subject);

  stralloc_init(&out);
  dfa_replace(re, rc, subject, subjlen, 1, global, repl_out, &out, &nreplaced);

  if(nreplaced)
    awk_lvalue_set_str(st, &lv, out.s ? out.s : "", out.len, 0);

  stralloc_free(&out);
  dfa_repl_free(rc);

  if(dynamic)
    dfa_free(re);

  return num_cell((double)nreplaced);
}

static awk_cell
do_split(struct awk_state* st, struct anode* args) {
  awk_cell sv = arg_eval(st, &args);
  const char* s = awk_tostr(st, &sv, 0);
  size_t slen = str_len(s);
  struct anode* arrnode = args;
  struct anode* fsnode;
  hashmap* arr;
  struct awk_span* spans = NULL;
  size_t spancap = 0, nf, i;
  size_t it = 0;
  hashentry* e;

  if(!arrnode || arrnode->op != A_VAR) {
    awk_runtime_error(st, "split(): second argument must be an array");
    return num_cell(0);
  }

  args = args->next;
  fsnode = args;
  arr = awk_array_of(st, var_cell(st, arrnode));

  if(!arr)
    return num_cell(0);

  while((e = hashmap_next(arr, &it))) {
    awk_cell_free((awk_cell*)e->val);
    alloc_free(e->val);
  }

  hashmap_clear(arr);

  if(fsnode && fsnode->op == A_REGEX) {
    nf = awk_split_re(st, s, slen, fsnode->u.re, &spans, &spancap);
  } else if(fsnode) {
    awk_cell fv = awk_eval(st, fsnode);
    const char* fs = awk_tostr(st, &fv, 0);

    nf = awk_split(st, s, slen, fs, str_len(fs), &spans, &spancap);
  } else {
    awk_cell* fsg = awk_global(st, SP_FS);
    const char* fs = fsg->str ? fsg->str : " ";

    nf = awk_split(st, s, slen, fs, fsg->str ? str_len(fsg->str) : 1, &spans, &spancap);
  }

  for(i = 0; i < nf; i++) {
    char key[32];
    size_t klen = fmt_ulong(key, (unsigned long)(i + 1));
    double v;
    int numeric = awk_looks_numeric(spans[i].s, spans[i].len, &v);
    awk_cell* c = alloc_zero(sizeof(awk_cell));

    awk_cell_set_str(c, spans[i].s, spans[i].len, numeric);

    if(numeric)
      c->num = v;

    hashmap_put2(arr, key, klen, c);
  }

  alloc_free(spans);
  return num_cell((double)nf);
}

static awk_cell
do_match(struct awk_state* st, struct anode* args) {
  awk_cell sv = arg_eval(st, &args);
  const char* s = awk_tostr(st, &sv, 0);
  size_t slen = str_len(s);
  struct dfa scratch, *re;
  int dynamic;
  struct dfa_span m;
  int found;

  if(!args || !compile_regex_arg(st, args, &scratch, &re, &dynamic))
    return num_cell(0);

  found = dfa_search(re, s, slen, 0, &m) == 1;

  if(dynamic)
    dfa_free(re);

  if(found) {
    awk_cell_set_num(awk_global(st, SP_RSTART), (double)(m.start + 1));
    awk_cell_set_num(awk_global(st, SP_RLENGTH), (double)(m.end - m.start));
    return num_cell((double)(m.start + 1));
  }

  awk_cell_set_num(awk_global(st, SP_RSTART), 0);
  awk_cell_set_num(awk_global(st, SP_RLENGTH), -1);
  return num_cell(0);
}

static awk_cell
do_case(struct awk_state* st, struct anode* args, int upper) {
  awk_cell sv = arg_eval(st, &args);
  const char* s = awk_tostr(st, &sv, 0);
  size_t n = str_len(s);
  awk_cell c = str_cell_tmp(st, s, n);

  if(upper)
    byte_upper(c.str, n);
  else
    byte_lower(c.str, n);

  return c;
}

awk_cell
awk_call_builtin(struct awk_state* st, struct anode* n) {
  struct anode* args = n->a;

  switch(n->idx) {
  case BI_LENGTH: {
    if(!args) {
      awk_cell f0 = *awk_rec_field(st, 0);

      return num_cell((double)str_len(f0.str ? f0.str : ""));
    }

    if(args->op == A_VAR) {
      awk_cell* c = var_cell(st, args);

      if(c->type == CELL_ARRAY)
        return num_cell((double)c->arr->used);
    }

    {
      awk_cell v = awk_eval(st, args);
      const char* s = awk_tostr(st, &v, 0);

      return num_cell((double)str_len(s));
    }
  }

  case BI_SUBSTR: {
    awk_cell sv = arg_eval(st, &args);
    const char* s = awk_tostr(st, &sv, 0);
    size_t len = str_len(s);
    awk_cell mv = arg_eval(st, &args);
    double m = floor(awk_tonum(st, &mv) + 0.5);
    long lo = (long)m, hi;

    if(args) {
      awk_cell nv = arg_eval(st, &args);
      double want = floor(awk_tonum(st, &nv) + 0.5);

      hi = (want > (double)(len) * 2 + 4) ? (long)len + 1 : lo + (long)want;
    } else {
      hi = (long)len + 1;
    }

    if(lo < 1)
      lo = 1;

    if(hi > (long)len + 1)
      hi = (long)len + 1;

    if(hi <= lo)
      return str_cell_tmp(st, "", 0);

    return str_cell_tmp(st, s + (lo - 1), (size_t)(hi - lo));
  }

  case BI_INDEX: {
    awk_cell sv = arg_eval(st, &args);
    const char* s = awk_tostr(st, &sv, 0);
    awk_cell tv = arg_eval(st, &args);
    const char* t = awk_tostr(st, &tv, 0);
    size_t slen = str_len(s), tlen = str_len(t), i;

    if(tlen == 0 || tlen > slen)
      return num_cell(0);

    for(i = 0; i + tlen <= slen; i++)
      if(!byte_diff(s + i, tlen, t))
        return num_cell((double)(i + 1));

    return num_cell(0);
  }

  case BI_SPLIT: return do_split(st, args);
  case BI_SUB: return do_sub(st, args, 0);
  case BI_GSUB: return do_sub(st, args, 1);
  case BI_MATCH: return do_match(st, args);

  case BI_SPRINTF: {
    stralloc out;
    awk_cell result;

    stralloc_init(&out);

    if(args) {
      awk_cell fv = awk_eval(st, args);
      const char* fmt = awk_tostr(st, &fv, 0);

      awk_sprintf(st, &out, fmt, str_len(fmt), args->next);
    }

    result = str_cell_tmp(st, out.s ? out.s : "", out.len);
    stralloc_free(&out);
    return result;
  }

  case BI_SIN: { awk_cell v = arg_eval(st, &args); return num_cell(sin(awk_tonum(st, &v))); }
  case BI_COS: { awk_cell v = arg_eval(st, &args); return num_cell(cos(awk_tonum(st, &v))); }

  case BI_ATAN2: {
    awk_cell y = arg_eval(st, &args), x = arg_eval(st, &args);

    return num_cell(atan2(awk_tonum(st, &y), awk_tonum(st, &x)));
  }

  case BI_EXP: { awk_cell v = arg_eval(st, &args); return num_cell(exp(awk_tonum(st, &v))); }
  case BI_LOG: { awk_cell v = arg_eval(st, &args); return num_cell(log(awk_tonum(st, &v))); }
  case BI_SQRT: { awk_cell v = arg_eval(st, &args); return num_cell(sqrt(awk_tonum(st, &v))); }

  case BI_INT: {
    awk_cell v = arg_eval(st, &args);
    double d = awk_tonum(st, &v);

    return num_cell(d < 0 ? ceil(d) : floor(d));
  }

  case BI_RAND: {
    /* xorshift64*; deterministic from seed 0 unless srand() was called */
    st->seed ^= st->seed << 13;
    st->seed ^= st->seed >> 7;
    st->seed ^= st->seed << 17;
    return num_cell((double)((st->seed >> 11) & ((1ULL << 53) - 1)) / (double)(1ULL << 53));
  }

  case BI_SRAND: {
    unsigned long prev = st->last_seed;

    if(args) {
      awk_cell v = arg_eval(st, &args);

      st->last_seed = (unsigned long)awk_tonum(st, &v);
    } else {
      st->last_seed = (unsigned long)time(NULL);
    }

    st->seed = st->last_seed ? st->last_seed : 1; /* xorshift needs a nonzero state */
    return num_cell((double)prev);
  }

  case BI_TOLOWER: return do_case(st, args, 0);
  case BI_TOUPPER: return do_case(st, args, 1);

  case BI_SYSTEM: {
    awk_cell v = arg_eval(st, &args);
    const char* cmd = awk_tostr(st, &v, 0);
    int status = -1;

    awk_streams_flush_all(st);

    if(st->io->run_shell)
      st->io->run_shell(st->io->ctx, cmd, 0, NULL, &status);
    else
      awk_runtime_error(st, "system() is not supported in this build");

    return num_cell((double)status);
  }

  case BI_CLOSE: {
    awk_cell v = arg_eval(st, &args);
    const char* name = awk_tostr(st, &v, 0);

    return num_cell(awk_stream_close(st, name) ? 0 : -1);
  }

  case BI_FFLUSH: {
    if(!args) {
      awk_streams_flush_all(st);
    } else {
      awk_cell v = arg_eval(st, &args);
      const char* name = awk_tostr(st, &v, 0);
      size_t i;

      for(i = 0; i < st->nstreams; i++)
        if(st->streams[i].is_write && !str_diff(st->streams[i].name, name))
          st->io->write(st->io->ctx, st->streams[i].h, "", 0);
    }

    return num_cell(0);
  }

  default: return num_cell(0);
  }
}
