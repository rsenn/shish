/* the interpreter: awk_eval() (expressions), awk_exec() (statements
 * and control flow), lvalues, and user function calls.
 *
 * Ownership convention for awk_cell values returned *by value* (from
 * awk_eval(), awk_lvalue_get() is the one exception -- it returns a
 * pointer into real storage): the .str they carry is always either
 * (a) a permanent arena string (a parsed string literal) or (b) an
 * alias into some owned storage (a variable/field/array element) or
 * (c) freshly computed into st->tmp, the per-statement arena. None of
 * these are ever freed by the consumer; only owned storage cells
 * (globals, frame locals, array elements, fields) are freed, with
 * awk_cell_free(), when overwritten or torn down. This is why eval()
 * has no matching "free the result" step anywhere below.
 * ----------------------------------------------------------------------- */
#include "awk_internal.h"
#include "../../lib/str.h"
#include "../../lib/byte.h"
#include "../../lib/alloc.h"
#include <math.h> /* pow, fmod */

static awk_cell
uninit_cell(void) {
  awk_cell c;

  byte_zero(&c, sizeof(c));
  return c;
}

static awk_cell
num_cell(double n) {
  awk_cell c;

  byte_zero(&c, sizeof(c));
  c.type = CELL_NUM;
  c.num = n;
  return c;
}

/* joins a[0..alen) and b[0..blen) into a fresh st->tmp buffer */
static const char*
concat_tmp(struct awk_state* st, const char* a, size_t alen, const char* b, size_t blen,
           size_t* outlen) {
  char* buf = arena_alloc(&st->tmp, alen + blen + 1, 1);

  *outlen = alen + blen;

  if(!buf)
    return "";

  byte_copy(buf, alen, a);
  byte_copy(buf + alen, blen, b);
  buf[alen + blen] = 0;
  return buf;
}

static awk_cell*
resolve_var(struct awk_state* st, struct anode* n) {
  if(n->flags & F_LOCAL)
    return st->frame->slot[n->idx];

  if(n->idx == SP_NF)
    awk_rec_ensure_split(st); /* NF is only accurate once $0 has been split */

  return awk_global(st, n->idx);
}

/* subscript list -> a SUBSEP-joined key, in freshly alloc()'d scratch
   (hashmap_put2/get2 copy what they need out of it immediately). */
static char*
build_subkey(struct awk_state* st, struct anode* list, size_t* outlen) {
  stralloc sa;
  const char* subsep;
  struct anode* it;
  char* r;

  stralloc_init(&sa);
  subsep = awk_global(st, SP_SUBSEP)->str;

  for(it = list; it; it = it->next) {
    awk_cell v = awk_eval(st, it);
    const char* s = awk_tostr(st, &v, 0);

    if(it != list)
      stralloc_cats(&sa, subsep ? subsep : "\034");

    stralloc_cats(&sa, s);
  }

  *outlen = sa.len;
  r = str_ndup(sa.s ? sa.s : "", sa.len);
  stralloc_free(&sa);
  return r;
}

static awk_cell*
array_element(struct awk_state* st, struct anode* arrvar, struct anode* sublist, int create) {
  awk_cell* av = resolve_var(st, arrvar);
  hashmap* arr = awk_array_of(st, av);
  size_t klen;
  char* key;
  awk_cell* c;

  if(!arr)
    return NULL;

  key = build_subkey(st, sublist, &klen);
  c = hashmap_get2(arr, key, klen);

  if(!c && create) {
    c = alloc_zero(sizeof(awk_cell));
    hashmap_put2(arr, key, klen, c);
  }

  alloc_free(key);
  return c;
}

struct awk_lvalue
awk_lvalue(struct awk_state* st, struct anode* n) {
  struct awk_lvalue lv;

  byte_zero(&lv, sizeof(lv));

  if(n->op == A_FIELD) {
    awk_cell idxv = awk_eval(st, n->a);

    lv.kind = LV_FIELD;
    lv.fieldn = (long)awk_tonum(st, &idxv);
    return lv;
  }

  if(n->op == A_INDEX) {
    lv.kind = LV_CELL;
    lv.cell = array_element(st, n->a, n->b, 1);
    return lv;
  }

  lv.kind = LV_CELL;
  lv.cell = resolve_var(st, n);
  return lv;
}

awk_cell*
awk_lvalue_get(struct awk_state* st, struct awk_lvalue* lv) {
  if(lv->kind == LV_FIELD)
    return awk_rec_field(st, lv->fieldn);

  return lv->cell;
}

void
awk_lvalue_set_num(struct awk_state* st, struct awk_lvalue* lv, double n) {
  if(lv->kind == LV_FIELD) {
    if(lv->fieldn == 0) {
      awk_cell* convfmt = awk_global(st, SP_CONVFMT);
      const char* s = awk_num2str(st, n, convfmt->str ? convfmt->str : "%.6g");

      awk_rec_setline(st, s, str_len(s));
    } else {
      awk_cell_set_num(awk_rec_field_for_write(st, lv->fieldn), n);
    }

    return;
  }

  if(lv->cell == awk_global(st, SP_NF)) {
    awk_rec_setnf(st, (long)n);
    return;
  }

  awk_cell_set_num(lv->cell, n);
}

void
awk_lvalue_set_str(struct awk_state* st, struct awk_lvalue* lv, const char* s, size_t len, int strnum) {
  if(lv->kind == LV_FIELD) {
    if(lv->fieldn == 0) {
      awk_rec_setline(st, s, len);
    } else {
      awk_cell_set_str(awk_rec_field_for_write(st, lv->fieldn), s, len, strnum);
    }

    return;
  }

  if(lv->cell == awk_global(st, SP_NF)) {
    double v;

    awk_looks_numeric(s, len, &v);
    awk_rec_setnf(st, (long)v);
    return;
  }

  awk_cell_set_str(lv->cell, s, len, strnum);
}

static void
lvalue_set_cell(struct awk_state* st, struct awk_lvalue* lv, awk_cell* val) {
  switch(val->type) {
  case CELL_NUM: awk_lvalue_set_num(st, lv, val->num); break;
  case CELL_STR: awk_lvalue_set_str(st, lv, val->str ? val->str : "", val->str ? str_len(val->str) : 0, 0); break;
  case CELL_STRNUM: awk_lvalue_set_str(st, lv, val->str, str_len(val->str), 1); break;
  case CELL_ARRAY: awk_runtime_error(st, "can't assign an array to a scalar"); break;
  default: awk_lvalue_set_str(st, lv, "", 0, 0); break;
  }
}

/* ---- expressions -------------------------------------------------- */

awk_cell
awk_eval(struct awk_state* st, struct anode* n) {
  if(!n || st->unwind)
    return uninit_cell();

  switch(n->op) {
  case A_NUM: return num_cell(n->u.num);
  case A_STR: {
    awk_cell c = uninit_cell();

    c.type = CELL_STR;
    c.str = n->u.str;
    return c;
  }

  case A_REGEX: {
    awk_cell f0 = *awk_rec_field(st, 0);

    return num_cell(dfa_test(n->u.re, f0.str ? f0.str : "", f0.str ? str_len(f0.str) : 0));
  }

  case A_VAR: return *resolve_var(st, n);

  case A_INDEX: {
    awk_cell* c = array_element(st, n->a, n->b, 1);

    return c ? *c : uninit_cell();
  }

  case A_FIELD: {
    awk_cell idxv = awk_eval(st, n->a);

    return *awk_rec_field(st, (long)awk_tonum(st, &idxv));
  }

  case A_ASSIGNOP: {
    struct awk_lvalue lv = awk_lvalue(st, n->a);
    awk_cell rhs = awk_eval(st, n->b);

    if(n->idx == 0) {
      lvalue_set_cell(st, &lv, &rhs);
      return rhs;
    }

    {
      awk_cell* cur = awk_lvalue_get(st, &lv);
      double a = awk_tonum(st, cur), b = awk_tonum(st, &rhs), r = 0;

      switch(n->idx) {
      case ADDOP_ADD: r = a + b; break;
      case ADDOP_SUB: r = a - b; break;
      case ADDOP_MUL: r = a * b; break;
      case ADDOP_DIV:
        if(b == 0) { awk_runtime_error(st, "division by zero"); r = 0; } else r = a / b;
        break;
      case ADDOP_MOD:
        if(b == 0) { awk_runtime_error(st, "division by zero"); r = 0; } else r = fmod(a, b);
        break;
      case ADDOP_POW: r = pow(a, b); break;
      }

      awk_lvalue_set_num(st, &lv, r);
      return num_cell(r);
    }
  }

  case A_INCDEC: {
    struct awk_lvalue lv = awk_lvalue(st, n->a);
    awk_cell* cur = awk_lvalue_get(st, &lv);
    double v = awk_tonum(st, cur);
    double nv = v + ((n->idx & 2) ? -1 : 1);

    awk_lvalue_set_num(st, &lv, nv);
    return num_cell((n->idx & 1) ? nv : v); /* bit0 = pre */
  }

  case A_NOT: { awk_cell a = awk_eval(st, n->a); return num_cell(!awk_tobool(st, &a)); }
  case A_UMINUS: { awk_cell a = awk_eval(st, n->a); return num_cell(-awk_tonum(st, &a)); }
  case A_UPLUS: { awk_cell a = awk_eval(st, n->a); return num_cell(+awk_tonum(st, &a)); }

  case A_POW: {
    awk_cell a = awk_eval(st, n->a), b = awk_eval(st, n->b);

    return num_cell(pow(awk_tonum(st, &a), awk_tonum(st, &b)));
  }

  case A_MUL: {
    awk_cell a = awk_eval(st, n->a), b = awk_eval(st, n->b);

    return num_cell(awk_tonum(st, &a) * awk_tonum(st, &b));
  }

  case A_DIV: {
    awk_cell a = awk_eval(st, n->a), b = awk_eval(st, n->b);
    double bv = awk_tonum(st, &b);

    if(bv == 0) {
      awk_runtime_error(st, "division by zero");
      return num_cell(0);
    }

    return num_cell(awk_tonum(st, &a) / bv);
  }

  case A_MOD: {
    awk_cell a = awk_eval(st, n->a), b = awk_eval(st, n->b);
    double bv = awk_tonum(st, &b);

    if(bv == 0) {
      awk_runtime_error(st, "division by zero");
      return num_cell(0);
    }

    return num_cell(fmod(awk_tonum(st, &a), bv));
  }

  case A_ADD: {
    awk_cell a = awk_eval(st, n->a), b = awk_eval(st, n->b);

    return num_cell(awk_tonum(st, &a) + awk_tonum(st, &b));
  }

  case A_SUB: {
    awk_cell a = awk_eval(st, n->a), b = awk_eval(st, n->b);

    return num_cell(awk_tonum(st, &a) - awk_tonum(st, &b));
  }

  case A_CONCAT: {
    awk_cell a = awk_eval(st, n->a);
    const char* as = awk_tostr(st, &a, 0);
    size_t alen = str_len(as);
    awk_cell b = awk_eval(st, n->b);
    const char* bs = awk_tostr(st, &b, 0);
    size_t blen = str_len(bs), rlen;
    const char* r = concat_tmp(st, as, alen, bs, blen, &rlen);
    awk_cell c = uninit_cell();

    c.type = CELL_STR;
    c.str = (char*)r;
    return c;
  }

  case A_CMP: {
    awk_cell a = awk_eval(st, n->a), b = awk_eval(st, n->b);
    int c = awk_cmp(st, &a, &b);

    switch(n->idx) {
    case CMP_LT: return num_cell(c < 0);
    case CMP_LE: return num_cell(c <= 0);
    case CMP_GT: return num_cell(c > 0);
    case CMP_GE: return num_cell(c >= 0);
    case CMP_EQ: return num_cell(c == 0);
    default: return num_cell(c != 0);
    }
  }

  case A_MATCH: {
    awk_cell a = awk_eval(st, n->a);
    const char* s = awk_tostr(st, &a, 0);
    size_t slen = str_len(s);
    int matched;

    if(n->b->op == A_REGEX) {
      matched = dfa_test(n->b->u.re, s, slen);
    } else {
      awk_cell rv = awk_eval(st, n->b);
      const char* pat = awk_tostr(st, &rv, 0);
      struct dfa re;

      byte_zero(&re, sizeof(re));

      if(dfa_compile(&re, pat, str_len(pat), DFA_ERE) != DFA_OK) {
        awk_runtime_error(st, "invalid dynamic regular expression");
        matched = 0;
      } else {
        matched = dfa_test(&re, s, slen);
        dfa_free(&re);
      }
    }

    return num_cell(n->idx ? !matched : matched);
  }

  case A_IN: {
    awk_cell* av = resolve_var(st, n->b);
    hashmap* arr = awk_array_of(st, av);
    size_t klen;
    char* key;
    int found;

    if(!arr)
      return num_cell(0);

    key = build_subkey(st, n->a, &klen);
    found = hashmap_get2(arr, key, klen) != NULL;
    alloc_free(key);
    return num_cell(found);
  }

  case A_AND: {
    awk_cell a = awk_eval(st, n->a);

    if(!awk_tobool(st, &a))
      return num_cell(0);

    {
      awk_cell b = awk_eval(st, n->b);

      return num_cell(awk_tobool(st, &b));
    }
  }

  case A_OR: {
    awk_cell a = awk_eval(st, n->a);

    if(awk_tobool(st, &a))
      return num_cell(1);

    {
      awk_cell b = awk_eval(st, n->b);

      return num_cell(awk_tobool(st, &b));
    }
  }

  case A_COND: {
    awk_cell c = awk_eval(st, n->a);

    return awk_eval(st, awk_tobool(st, &c) ? n->b : n->c);
  }

  case A_CALL: {
    awk_cell out = uninit_cell();
    struct awk_func* fn = &st->prog->funcs[n->idx];

    awk_call_user(st, fn, n->a, &out);
    return out;
  }

  case A_CALLBUILTIN: return awk_call_builtin(st, n);

  case A_GETLINE: {
    awk_cell* setvar = NULL;
    int r = awk_getline_from(st, n, &setvar);

    return num_cell(r);
  }

  case A_GROUP: return awk_eval(st, n->a); /* stray grouping; see the parser */

  default: return uninit_cell();
  }
}

/* ---- statements ----------------------------------------------------- */

int
awk_exec(struct awk_state* st, struct anode* n) {
  if(!n)
    return CF_NORMAL;

  if(st->unwind)
    return CF_EXIT;

  switch(n->op) {
  case A_BLOCK: {
    struct anode* s;

    for(s = n->a; s; s = s->next) {
      int rc = awk_exec(st, s);

      if(rc != CF_NORMAL || st->unwind)
        return st->unwind ? CF_EXIT : rc;
    }

    return CF_NORMAL;
  }

  case A_IF: {
    awk_cell c = awk_eval(st, n->a);

    if(st->unwind)
      return CF_EXIT;

    if(awk_tobool(st, &c))
      return awk_exec(st, n->b);

    return awk_exec(st, n->c);
  }

  case A_WHILE: {
    for(;;) {
      awk_cell c = awk_eval(st, n->a);
      int rc;

      if(st->unwind || !awk_tobool(st, &c))
        break;

      rc = awk_exec(st, n->b);

      if(st->unwind)
        return CF_EXIT;

      if(rc == CF_BREAK)
        break;

      if(rc != CF_NORMAL && rc != CF_CONTINUE)
        return rc;
    }

    return CF_NORMAL;
  }

  case A_DOWHILE: {
    for(;;) {
      int rc = awk_exec(st, n->a);
      awk_cell c;

      if(st->unwind)
        return CF_EXIT;

      if(rc == CF_BREAK)
        break;

      if(rc != CF_NORMAL && rc != CF_CONTINUE)
        return rc;

      c = awk_eval(st, n->b);

      if(st->unwind || !awk_tobool(st, &c))
        break;
    }

    return CF_NORMAL;
  }

  case A_FOR: {
    if(n->a) {
      awk_exec(st, n->a);

      if(st->unwind)
        return CF_EXIT;
    }

    for(;;) {
      int rc;

      if(n->b) {
        awk_cell c = awk_eval(st, n->b);

        if(st->unwind)
          return CF_EXIT;

        if(!awk_tobool(st, &c))
          break;
      }

      rc = awk_exec(st, n->d);

      if(st->unwind)
        return CF_EXIT;

      if(rc == CF_BREAK)
        break;

      if(rc != CF_NORMAL && rc != CF_CONTINUE)
        return rc;

      if(n->c) {
        awk_exec(st, n->c);

        if(st->unwind)
          return CF_EXIT;
      }
    }

    return CF_NORMAL;
  }

  case A_FORIN: {
    awk_cell* av = resolve_var(st, n->b);
    hashmap* arr = awk_array_of(st, av);
    struct awk_lvalue lv = awk_lvalue(st, n->a);
    size_t it = 0;
    hashentry* e;

    if(!arr)
      return CF_NORMAL;

    while((e = hashmap_next(arr, &it))) {
      int rc;
      double v;
      int numeric = awk_looks_numeric(e->key, e->keylen, &v);

      awk_lvalue_set_str(st, &lv, e->key, e->keylen, numeric);
      rc = awk_exec(st, n->c);

      if(st->unwind)
        return CF_EXIT;

      if(rc == CF_BREAK)
        break;

      if(rc != CF_NORMAL && rc != CF_CONTINUE)
        return rc;
    }

    return CF_NORMAL;
  }

  case A_BREAK: return CF_BREAK;
  case A_CONTINUE: return CF_CONTINUE;
  case A_NEXT: return CF_NEXT;
  case A_NEXTFILE: return CF_NEXTFILE;

  case A_EXIT: {
    if(n->a) {
      awk_cell c = awk_eval(st, n->a);

      st->exit_status = (int)awk_tonum(st, &c);
    }

    return CF_EXIT;
  }

  case A_RETURN: {
    st->retval = n->a ? awk_eval(st, n->a) : uninit_cell();
    return CF_RETURN;
  }

  case A_DELETE: {
    awk_cell* av = resolve_var(st, n->a);
    hashmap* arr = awk_array_of(st, av);

    if(!arr)
      return CF_NORMAL;

    if(n->b) {
      size_t klen;
      char* key = build_subkey(st, n->b, &klen);
      hashentry* e = hashmap_get_entry(arr, key, klen);

      if(e) {
        awk_cell_free((awk_cell*)e->val);
        alloc_free(e->val);
        hashmap_delete2(arr, key, klen);
      }

      alloc_free(key);
    } else {
      size_t it = 0;
      hashentry* e;

      while((e = hashmap_next(arr, &it))) {
        awk_cell_free((awk_cell*)e->val);
        alloc_free(e->val);
      }

      hashmap_clear(arr);
    }

    return CF_NORMAL;
  }

  case A_PRINT:
  case A_PRINTF: {
    stralloc out;
    struct anode* args = n->a;

    stralloc_init(&out);

    if(n->op == A_PRINT) {
      const char* ofs = awk_global(st, SP_OFS)->str;
      const char* ors = awk_global(st, SP_ORS)->str;
      struct anode* it;

      if(!args) {
        awk_cell f0 = *awk_rec_field(st, 0);

        stralloc_cats(&out, f0.str ? f0.str : "");
      } else {
        for(it = args; it; it = it->next) {
          awk_cell v = awk_eval(st, it);

          if(it != args)
            stralloc_cats(&out, ofs ? ofs : " ");

          stralloc_cats(&out, awk_tostr(st, &v, 1));
        }
      }

      stralloc_cats(&out, ors ? ors : "\n");
    } else {
      if(args) {
        awk_cell fmtv = awk_eval(st, args);
        const char* fmt = awk_tostr(st, &fmtv, 0);

        awk_sprintf(st, &out, fmt, str_len(fmt), args->next);
      }
    }

    if(n->idx == REDIR_NONE) {
      awk_output(st, out.s ? out.s : "", out.len);
    } else {
      awk_cell tv = awk_eval(st, n->b);
      const char* target = awk_tostr(st, &tv, 0);
      struct awk_stream* strm =
          awk_stream_for_write(st, target, n->idx == REDIR_APPEND, n->idx == REDIR_PIPE);

      if(strm)
        st->io->write(st->io->ctx, strm->h, out.s ? out.s : "", out.len);
    }

    stralloc_free(&out);
    return st->unwind ? CF_EXIT : CF_NORMAL;
  }

  case A_EXPRSTMT: {
    awk_eval(st, n->a);
    return st->unwind ? CF_EXIT : CF_NORMAL;
  }

  default: return CF_NORMAL;
  }
}

int
awk_call_user(struct awk_state* st, struct awk_func* fn, struct anode* args, awk_cell* out) {
  struct awk_frame frame, *saved_frame = st->frame;
  awk_cell** slots;
  awk_cell* locals;
  struct anode* a;
  size_t i;
  int rc;

  byte_zero(out, sizeof(*out));

  if(st->depth >= AWK_MAXDEPTH) {
    awk_runtime_error(st, "function call nesting too deep");
    return CF_NORMAL;
  }

  slots = fn->nparams ? alloc(fn->nparams * sizeof(awk_cell*)) : NULL;
  locals = fn->nparams ? alloc_zero(fn->nparams * sizeof(awk_cell)) : NULL;

  for(i = 0, a = args; i < fn->nparams; i++) {
    if(a && (a->op == A_VAR)) {
      awk_cell* src = resolve_var(st, a);

      if(src->type == CELL_ARRAY || src->type == CELL_UNINIT) {
        /* shares the caller's cell: an array is always by reference,
           and an uninitialised argument may *become* one inside the
           callee, which must be visible back in the caller too */
        slots[i] = src;
        a = a->next;
        continue;
      }
    }

    if(a) {
      awk_cell v = awk_eval(st, a);

      awk_cell_assign(&locals[i], &v);
      a = a->next;
    }

    slots[i] = &locals[i];
  }

  frame.slot = slots;
  frame.local = locals;
  frame.n = fn->nparams;

  st->frame = &frame;
  st->depth++;

  rc = awk_exec(st, fn->body);

  if(rc == CF_RETURN) {
    if(st->retval.type == CELL_STR || st->retval.type == CELL_STRNUM) {
      size_t l = str_len(st->retval.str);
      char* copy = arena_alloc(&st->tmp, l + 1, 1);

      if(copy) {
        byte_copy(copy, l, st->retval.str);
        copy[l] = 0;
      }

      out->type = st->retval.type;
      out->str = copy ? copy : (char*)"";
    } else if(st->retval.type == CELL_ARRAY) {
      awk_runtime_error(st, "can't return an array");
    } else {
      out->type = st->retval.type;
      out->num = st->retval.num;
    }
  }

  st->depth--;
  st->frame = saved_frame;

  for(i = 0; i < fn->nparams; i++)
    if(slots[i] == &locals[i])
      awk_cell_free(&locals[i]);

  alloc_free(locals);
  alloc_free(slots);

  if(rc == CF_EXIT)
    st->unwind = UNWIND_EXIT;

  return rc;
}
