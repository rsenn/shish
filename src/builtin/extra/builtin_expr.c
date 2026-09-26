#include "../../../lib/uint64.h"
#include "../../trace.h"
#include "../../../lib/byte.h"
#include "../../../lib/fmt.h"
#include "../../../lib/shell.h"
#include "../../../lib/str.h"
#include "../../../lib/stralloc.h"
#include "../../../lib/buffer.h"
#include "../../fdtable.h"
#include "../../expand.h"
#include "../../fdstack.h"
#include "../../parse.h"
#include "../../source.h"
#include "../../tree.h"
#include "../../debug.h"
#include <ctype.h>

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
                         "    expression            evaluate an arithmetic/logical expression\n";

int
builtin_expr(int argc, char* argv[]) {
  struct fd fd;
  struct source src;
  struct parser p;
  union node* expr;
  int ret = 0;
  int64 result = 0;
  stralloc sa;
  stralloc_init(&sa);

  if(argc == 1) {
    ret = 1;
  } else if(!str_diff(argv[1], "length")) {
    result = argv[2] ? str_len(argv[2]) : 0;

  } else if(!str_diff(argv[1], "index")) {
    const char* haystack = argc >= 3 ? argv[2] : "";
    const char* needle = argc >= 4 ? argv[3] : "";
    size_t i, n = str_len(haystack) - str_len(needle);

    for(i = 0; i < n; i++) {
      if(!byte_diff(&haystack[i], str_len(needle), needle)) {
        result = i + 1;
        break;
      }
    }

  } else if(argc == 4 && !str_diff(argv[2], ":")) {
    stralloc match;

    ret = expr_match(argv[1], argv[3], &match);

    buffer_put(fd_out->w, match.s, match.len);
    buffer_putnlflush(fd_out->w);

    stralloc_free(&match);
    stralloc_free(&sa);
    return ret;

  } else {
    int i;

    /* concatenate all arguments following the "expr", separated by a
       whitespace and terminated by a newline */
    for(i = 1; i < argc; i++) {
      if(i > 1)
        stralloc_catc(&sa, ' ');

      stralloc_cats(&sa, argv[i]);
    }

    /* create a new i/o context and initialize a parser */
    source_buffer(&src, &fd, sa.s, sa.len);
    parse_init(&p, P_ARITH | P_NOREDIR);

    /* parse the string as a compound list */
    if((expr = parse_arith_expr(&p))) {
      /*enum tok_flag tok =*/parse_gettok(&p, P_SKIPNL);

      TRACE(TRACE_BUILTIN, "expr.tree", trace_kind("kind", expr->id));

      if(expand_arith_expr(expr, &result)) {
        ret = 1;
      }

      tree_free(expr);
    }

    source_popfd(&fd);
  }

  if(ret == 0) {
    buffer_putlonglong(fd_out->w, result);
    buffer_putnlflush(fd_out->w);
  }

  return ret;
}
