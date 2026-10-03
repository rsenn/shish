#include "../../../lib/uint64.h"
#include "../../builtin.h"
#include "../../trace.h"
#include "../../../lib/byte.h"
#include "../../../lib/fmt.h"
#include "../../../lib/shell.h"
#include "../../../lib/str.h"
#include "../../../lib/scan.h"
#include "../../../lib/stralloc.h"
#include "../../../lib/buffer.h"
#include "../../fdtable.h"

/* Define to use system <regex.h> instead of lib/bre.h */
/* #define USE_SYSTEM_REGEX */

#ifdef USE_SYSTEM_REGEX
#include <regex.h>
#else
#include "../../../text/dfa.h"
#endif

/* run "str : pat", filling *out with what POSIX says to print (first
 * capture group's text, or match length if no group; "" or "0" on no
 * match). Returns expr's exit status for the result: 0 unless it's
 * empty or "0".
 * ----------------------------------------------------------------------- */
static int
expr_match(const char* str, const char* pat, stralloc* out) {
  stralloc_init(out);

#ifdef USE_SYSTEM_REGEX
  regex_t re;
  regmatch_t pmatch[10];

  if(regcomp(&re, pat, 0) != 0) {
    stralloc_catc(out, '0');
    return 1;
  }

  /* POSIX expr is anchored to the beginning of the string */
  char* anchored_pat = malloc(str_len(pat) + 2);
  anchored_pat[0] = '^';
  str_copy(anchored_pat + 1, pat);
  regfree(&re);

  if(regcomp(&re, anchored_pat, 0) != 0) {
    free(anchored_pat);
    stralloc_catc(out, '0');
    return 1;
  }
  free(anchored_pat);

  if(regexec(&re, str, 10, pmatch, 0) != 0) {
    regfree(&re);
    stralloc_catc(out, '0');
    return 1;
  }

  if(re.re_nsub > 0 && pmatch[1].rm_so != -1) {
    stralloc_catb(out, str + pmatch[1].rm_so, (size_t)(pmatch[1].rm_eo - pmatch[1].rm_so));
  } else {
    char buf[FMT_ULONG];
    size_t match_len = (size_t)(pmatch[0].rm_eo - pmatch[0].rm_so);
    stralloc_catb(out, buf, fmt_ulong(buf, match_len));
  }

  regfree(&re);
#else
  struct dfa d;
  struct dfa_span m, g[1];
  long len;
  size_t str_n = str_len(str);

  byte_zero(&d, sizeof(d));

  /* a bad pattern has no match either way; POSIX leaves the result
     unspecified, so treat it the same as "didn't match" */
  if(dfa_compile(&d, pat, str_len(pat), 0) != DFA_OK) {
    stralloc_catc(out, '0');
    return 1;
  }

  len = dfa_prefix(&d, str, str_n);

  if(len < 0) {
    if(dfa_groups(&d) == 0)
      stralloc_catc(out, '0');

    dfa_free(&d);
    return 1;
  }

  if(dfa_groups(&d) > 0) {
    m.start = 0;
    m.end = (size_t)len;

    if(dfa_submatch(&d, str, str_n, &m, g, 1) && g[0].start != (size_t)-1)
      stralloc_catb(out, str + g[0].start, g[0].end - g[0].start);
  } else {
    char buf[FMT_ULONG];

    stralloc_catb(out, buf, fmt_ulong(buf, (size_t)len));
  }

  dfa_free(&d);
#endif

  return out->len == 0 || (out->len == 1 && out->s[0] == '0') ? 1 : 0;
}

/* POSIX expression grammar, lowest to highest precedence:
 *
 *   |   &   = > >= < <= !=   + -   * / %   :   ( expr )
 *
 * Every operand is a string; arithmetic needs integers, comparisons are
 * numeric when both sides are integers and string comparisons otherwise.
 * ----------------------------------------------------------------------- */
struct ex {
  char** a;   /* operands */
  int n, i;   /* count, next operand */
  int err;    /* 0, 2 = invalid expression, 3 = other error */
  char** av;  /* builtin argv, for diagnostics */
};

static void ex_or(struct ex* x, stralloc* out);

static int
ex_int(const char* s, int64* v) {
  size_t n = str_len(s);

  return n > 0 && scan_longlong(s, v) == n;
}

static int
ex_true(stralloc* v) {
  int64 n;

  if(v->len == 0)
    return 0;

  stralloc_nul(v);
  return !(ex_int(v->s, &n) && n == 0);
}

static void
ex_fail(struct ex* x, int err, const char* msg) {
  if(!x->err) {
    x->err = err;
    builtin_errmsg(x->av, (char*)msg, NULL);
  }
}

static void
ex_setint(stralloc* v, int64 n) {
  char buf[FMT_ULONG + 1];

  stralloc_zero(v);
  stralloc_catb(v, buf, fmt_longlong(buf, n));
}

static int
ex_peek(struct ex* x, const char* tok) {
  return x->i < x->n && !str_diff(x->a[x->i], tok);
}

static void
ex_primary(struct ex* x, stralloc* out) {
  stralloc_zero(out);

  if(x->i >= x->n) {
    ex_fail(x, 2, "syntax error: missing operand");
    return;
  }

  if(ex_peek(x, "(")) {
    x->i++;
    ex_or(x, out);

    if(!ex_peek(x, ")"))
      ex_fail(x, 2, "syntax error: expecting ')'");
    else
      x->i++;

    return;
  }

  stralloc_cats(out, x->a[x->i++]);
}

static void
ex_match(struct ex* x, stralloc* out) {
  ex_primary(x, out);

  while(!x->err && ex_peek(x, ":")) {
    stralloc pat, res;

    x->i++;
    stralloc_init(&pat);
    stralloc_init(&res);
    ex_primary(x, &pat);

    if(!x->err) {
      stralloc_nul(out);
      stralloc_nul(&pat);
      expr_match(out->s, pat.s, &res);
      stralloc_copy(out, &res);
    }

    stralloc_free(&res);
    stralloc_free(&pat);
  }
}

static void
ex_mul(struct ex* x, stralloc* out) {
  ex_match(x, out);

  while(!x->err && (ex_peek(x, "*") || ex_peek(x, "/") || ex_peek(x, "%"))) {
    char op = x->a[x->i++][0];
    stralloc rhs;
    int64 a, b;

    stralloc_init(&rhs);
    ex_match(x, &rhs);
    stralloc_nul(out);
    stralloc_nul(&rhs);

    if(x->err) {
    } else if(!ex_int(out->s, &a) || !ex_int(rhs.s, &b)) {
      ex_fail(x, 2, "non-integer argument");
    } else if(op != '*' && b == 0) {
      ex_fail(x, 3, "division by zero");
    } else {
      /* b == -1 would overflow INT64_MIN / -1 */
      ex_setint(out, op == '*' ? a * b : b == -1 ? (op == '/' ? -a : 0) : op == '/' ? a / b : a % b);
    }

    stralloc_free(&rhs);
  }
}

static void
ex_add(struct ex* x, stralloc* out) {
  ex_mul(x, out);

  while(!x->err && (ex_peek(x, "+") || ex_peek(x, "-"))) {
    char op = x->a[x->i++][0];
    stralloc rhs;
    int64 a, b;

    stralloc_init(&rhs);
    ex_mul(x, &rhs);
    stralloc_nul(out);
    stralloc_nul(&rhs);

    if(x->err) {
    } else if(!ex_int(out->s, &a) || !ex_int(rhs.s, &b)) {
      ex_fail(x, 2, "non-integer argument");
    } else {
      ex_setint(out, op == '+' ? a + b : a - b);
    }

    stralloc_free(&rhs);
  }
}

static void
ex_cmp(struct ex* x, stralloc* out) {
  static const char* const ops[] = {"=", "==", ">", ">=", "<", "<=", "!=", 0};

  ex_add(x, out);

  for(;;) {
    int k, c;
    stralloc rhs;
    int64 a, b;

    for(k = 0; ops[k]; k++)
      if(ex_peek(x, ops[k]))
        break;

    if(x->err || !ops[k])
      return;

    x->i++;
    stralloc_init(&rhs);
    ex_add(x, &rhs);
    stralloc_nul(out);
    stralloc_nul(&rhs);

    if(x->err) {
      stralloc_free(&rhs);
      return;
    }

    if(ex_int(out->s, &a) && ex_int(rhs.s, &b))
      c = a < b ? -1 : a > b;
    else
      c = str_diff(out->s, rhs.s);

    c = k <= 1 ? c == 0 : k == 2 ? c > 0 : k == 3 ? c >= 0 : k == 4 ? c < 0 : k == 5 ? c <= 0 : c != 0;
    ex_setint(out, c);
    stralloc_free(&rhs);
  }
}

static void
ex_and(struct ex* x, stralloc* out) {
  ex_cmp(x, out);

  while(!x->err && ex_peek(x, "&")) {
    stralloc rhs;

    x->i++;
    stralloc_init(&rhs);
    ex_cmp(x, &rhs);

    if(!x->err && !(ex_true(out) && ex_true(&rhs)))
      ex_setint(out, 0);

    stralloc_free(&rhs);
  }
}

static void
ex_or(struct ex* x, stralloc* out) {
  ex_and(x, out);

  while(!x->err && ex_peek(x, "|")) {
    stralloc rhs;

    x->i++;
    stralloc_init(&rhs);
    ex_and(x, &rhs);

    if(!x->err && !ex_true(out)) {
      if(!ex_true(&rhs))
        ex_setint(&rhs, 0);

      stralloc_copy(out, &rhs);
    }

    stralloc_free(&rhs);
  }
}

/* parse and evaluate arguments
 * ----------------------------------------------------------------------- */
const char help_expr[] = "    Evaluate an expression and print the result.\n"
                         "\n"
                         "    string : pattern     match string against a leading BRE pattern;\n"
                         "                         prints the first \\(...\\) group, or the\n"
                         "                         match length if the pattern has no group\n"
                         "    length string        print the length of string\n"
                         "    index string chars   print the first position in string of any\n"
                         "                         character in chars, 0 if none is found\n"
                         "    | & = > >= < <= != + - * / % ( )\n"
                         "                         POSIX operators; exit status is 0 for a result\n"
                         "                         that is neither empty nor 0, 1 for one that is,\n"
                         "                         2 for an invalid expression, 3 for an error\n";

int
builtin_expr(int argc, char* argv[]) {
  struct ex x;
  stralloc res;
  int ret;

  if(argc > 1 && !str_diff(argv[1], "--")) {
    argv++;
    argc--;
  }

  if(argc > 1 && !str_diff(argv[1], "length")) {
    int64 n = argv[2] ? str_len(argv[2]) : 0;

    buffer_putlonglong(fd_out->w, n);
    buffer_putnlflush(fd_out->w);
    return n == 0;
  }

  if(argc > 1 && !str_diff(argv[1], "index")) {
    const char* haystack = argc >= 3 ? argv[2] : "";
    const char* needle = argc >= 4 ? argv[3] : "";
    size_t i, n = str_len(haystack) - str_len(needle);
    int64 result = 0;

    for(i = 0; i < n; i++) {
      if(!byte_diff(&haystack[i], str_len(needle), needle)) {
        result = i + 1;
        break;
      }
    }

    buffer_putlonglong(fd_out->w, result);
    buffer_putnlflush(fd_out->w);
    return result == 0;
  }

  x.a = argv + 1;
  x.n = argc - 1;
  x.i = 0;
  x.err = 0;
  x.av = argv;
  stralloc_init(&res);

  ex_or(&x, &res);

  if(!x.err && x.i < x.n)
    ex_fail(&x, 2, "syntax error: unexpected argument");

  if(x.err) {
    ret = x.err;
  } else {
    buffer_put(fd_out->w, res.s, res.len);
    buffer_putnlflush(fd_out->w);
    ret = !ex_true(&res);
  }

  stralloc_free(&res);
  return ret;
}
