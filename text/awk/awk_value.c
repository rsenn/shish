/* cell (value) representation: conversions, comparison. See
 * awk_internal.h's awk_cell doc comment for the type states. */
#include "awk_internal.h"
#include "../../lib/str.h"
#include "../../lib/byte.h"
#include "../../lib/alloc.h"
#include "../../lib/fmt.h"
#include <stdio.h>  /* snprintf */
#include <stdlib.h> /* strtod */
#include <math.h>   /* fabs */

void
awk_cell_free(awk_cell* c) {
  alloc_free(c->str);

  if(c->type == CELL_ARRAY && c->arr) {
    size_t it = 0;
    hashentry* e;

    while((e = hashmap_next(c->arr, &it))) {
      awk_cell_free((awk_cell*)e->val);
      alloc_free(e->val);
      alloc_free(e->key);
    }

    alloc_free(c->arr->buckets);
    alloc_free(c->arr);
  }

  c->type = CELL_UNINIT;
  c->num = 0;
  c->str = NULL;
  c->arr = NULL;
}

void
awk_cell_set_num(awk_cell* c, double n) {
  alloc_free(c->str);
  c->str = NULL;
  c->type = CELL_NUM;
  c->num = n;
}

void
awk_cell_set_str(awk_cell* c, const char* s, size_t n, int strnum) {
  char* copy = str_ndup(s, n);

  alloc_free(c->str);
  c->str = copy;
  c->num = 0;
  c->type = strnum ? CELL_STRNUM : CELL_STR;
}

void
awk_cell_assign(awk_cell* dst, const awk_cell* src) {
  if(src->type == CELL_ARRAY) {
    /* the interpreter must not reach here (scalar-context checks
       happen before calling this); guard anyway rather than corrupt
       dst by copying a raw hashmap* it does not own. */
    return;
  }

  alloc_free(dst->str);

  if(dst->type == CELL_ARRAY) {
    /* overwriting an array with a scalar: same guard, other direction */
    return;
  }

  dst->type = src->type;
  dst->num = src->num;
  dst->str = src->str ? str_ndup(src->str, str_len(src->str)) : NULL;
}

int
awk_looks_numeric(const char* s, size_t n, double* out) {
  const char* p = s;
  const char* end = s + n;
  char* stop;
  char buf[64];
  double v;

  while(p < end && (*p == ' ' || *p == '\t' || *p == '\n'))
    p++;

  if(p >= end) {
    if(out)
      *out = 0;
    return 0; /* blank/empty is not a numeric string */
  }

  /* strtod needs a NUL-terminated buffer; awk numeric strings are
     short, so a bounded copy is simpler than a manual grammar. */
  {
    size_t len = (size_t)(end - p);

    if(len >= sizeof(buf)) {
      if(out)
        *out = 0;
      return 0;
    }

    byte_copy(buf, len, p);
    buf[len] = 0;
  }

  v = strtod(buf, &stop);

  if(stop == buf) {
    if(out)
      *out = 0;
    return 0;
  }

  while(*stop == ' ' || *stop == '\t' || *stop == '\n')
    stop++;

  if(*stop != 0) {
    if(out)
      *out = 0;
    return 0;
  }

  if(out)
    *out = v;

  return 1;
}

int
awk_is_numeric_cell(const awk_cell* c) {
  return c->type == CELL_NUM || c->type == CELL_STRNUM;
}

double
awk_tonum(struct awk_state* st, awk_cell* c) {
  switch(c->type) {
  case CELL_NUM: return c->num;
  case CELL_STR:
  case CELL_STRNUM: return strtod(c->str, NULL);
  case CELL_ARRAY: awk_runtime_error(st, "can't read value of an array"); return 0;
  default: return 0; /* CELL_UNINIT */
  }
}

/* is fmt a single "%[flags][width][.prec][eEfFgG]" spec and nothing
   else? CONVFMT/OFMT are user-settable strings handed straight to
   snprintf; this keeps a mismatched conversion (e.g. "%s") from
   reading garbage arguments. */
static int
safe_float_fmt(const char* fmt) {
  const char* p = fmt;

  if(*p++ != '%')
    return 0;

  while(*p == '-' || *p == '+' || *p == ' ' || *p == '0' || *p == '#')
    p++;

  while(*p >= '0' && *p <= '9')
    p++;

  if(*p == '.') {
    p++;

    while(*p >= '0' && *p <= '9')
      p++;
  }

  switch(*p) {
  case 'e': case 'E': case 'f': case 'F': case 'g': case 'G': break;
  default: return 0;
  }

  return p[1] == 0;
}

const char*
awk_num2str(struct awk_state* st, double n, const char* fmt) {
  char* buf;

  if(n == (double)(long long)n && fabs(n) < 1e18) {
    buf = arena_alloc(&st->tmp, 32, 1);

    if(buf) {
      size_t l = fmt_longlong(buf, (int64)(long long)n);

      buf[l] = 0;
    }

    return buf ? buf : "";
  }

  buf = arena_alloc(&st->tmp, 64, 1);

  if(!buf)
    return "";

  snprintf(buf, 64, safe_float_fmt(fmt) ? fmt : "%.6g", n);
  return buf;
}

const char*
awk_tostr(struct awk_state* st, awk_cell* c, int output) {
  switch(c->type) {
  case CELL_STR:
  case CELL_STRNUM: return c->str;

  case CELL_NUM: {
    awk_cell* fmtcell = awk_global(st, output ? SP_OFMT : SP_CONVFMT);

    return awk_num2str(st, c->num, fmtcell->str ? fmtcell->str : "%.6g");
  }

  case CELL_ARRAY: awk_runtime_error(st, "can't read value of an array"); return "";
  default: return ""; /* CELL_UNINIT */
  }
}

int
awk_tobool(struct awk_state* st, awk_cell* c) {
  switch(c->type) {
  case CELL_NUM: return c->num != 0;
  case CELL_STRNUM: return strtod(c->str, NULL) != 0;
  case CELL_STR: return c->str && c->str[0] != 0;
  case CELL_ARRAY: awk_runtime_error(st, "can't read value of an array"); return 0;
  default: return 0;
  }
}

int
awk_cmp(struct awk_state* st, awk_cell* a, awk_cell* b) {
  int an = awk_is_numeric_cell(a), bn = awk_is_numeric_cell(b);
  int au = (a->type == CELL_UNINIT), bu = (b->type == CELL_UNINIT);
  int numeric = (an && bn) || (an && bu) || (bn && au) || (au && bu);

  if(numeric) {
    double x = awk_tonum(st, a), y = awk_tonum(st, b);

    return (x < y) ? -1 : (x > y ? 1 : 0);
  }

  return str_diff(awk_tostr(st, a, 0), awk_tostr(st, b, 0));
}

awk_cell*
awk_global(struct awk_state* st, long idx) {
  return &st->globals[idx];
}

hashmap*
awk_array_of(struct awk_state* st, awk_cell* c) {
  if(c->type == CELL_ARRAY)
    return c->arr;

  if(c->type != CELL_UNINIT) {
    awk_runtime_error(st, "scalar used where an array was expected");
    return NULL;
  }

  c->arr = alloc(sizeof(hashmap));
  hashmap_init(c->arr);
  c->type = CELL_ARRAY;
  return c->arr;
}
