#include "../../lib/uint64.h"
#include "../../lib/byte.h"
#include "../../lib/fmt.h"
#include "../../lib/shell.h"
#include "../../lib/str.h"
#include "../../lib/stralloc.h"
#include "../../lib/buffer.h"
#include "../fdtable.h"
#include "../expand.h"
#include "../fdstack.h"
#include "../parse.h"
#include "../source.h"
#include "../tree.h"
#include "../debug.h"
#include <ctype.h>

/* "expr STRING : PATTERN" -- POSIX Basic Regular Expression matcher.
 *
 * Deliberately self-contained rather than linking libc <regex.h>: this
 * project avoids libc facilities generally (see CLAUDE.md), and several
 * of its cross-build targets (dietlibc, some Windows/embedded configs)
 * either lack POSIX regex entirely or only have a stripped-down one.
 *
 * Scope: literals, "." (any char), "*" (zero-or-more of the preceding
 * atom, literal if it has no preceding atom -- i.e. at the very start
 * of the pattern or right after "\("), bracket expressions ("[...]",
 * ranges, "^" negation, "[:class:]" -- collating symbols/equivalence
 * classes are not supported, unlike lib/path/path_fnmatch.c's glob
 * matcher), "^"/"$" anchors, and "\( \)" capture groups (up to 9,
 * though POSIX "expr" only ever reports the first). Matching is
 * anchored to the start of the string (expr's own semantics, not a
 * grep-style search) and greedy-backtracking, which gives the POSIX-
 * required leftmost-longest result for any pattern without "true"
 * alternation -- BRE proper doesn't have alternation at all (that's
 * an ERE/GNU "\|" extension), so this covers BRE as actually
 * specified.
 * ----------------------------------------------------------------------- */
#define BRE_MAXGROUPS 9

struct bre_ctx {
  const char* pat_start;
  const char* open_ptr[BRE_MAXGROUPS];  /* pattern position of each "\(", in order */
  const char* close_ptr[BRE_MAXGROUPS]; /* its matching "\)", once found */
  int nopen;
  const char* gstart[BRE_MAXGROUPS];
  const char* gend[BRE_MAXGROUPS];
};

/* one pass over the pattern to statically pair up "\(" with its "\)"
 * and assign group numbers by textual order -- matching() then just
 * looks up "which group does the \( / \) at this pattern position
 * belong to" instead of maintaining a runtime stack that would need
 * careful unwinding on every backtrack.
 * ----------------------------------------------------------------------- */
static void
bre_prepass(struct bre_ctx* ctx, const char* pat) {
  int stack[BRE_MAXGROUPS];
  int sp = 0;

  ctx->nopen = 0;

  for(; *pat; pat++) {
    if(pat[0] == '\\' && pat[1] == '(') {
      if(ctx->nopen < BRE_MAXGROUPS) {
        ctx->open_ptr[ctx->nopen] = pat;

        if(sp < BRE_MAXGROUPS)
          stack[sp++] = ctx->nopen;

        ctx->nopen++;
      }

      pat++;
    } else if(pat[0] == '\\' && pat[1] == ')') {
      if(sp > 0)
        ctx->close_ptr[stack[--sp]] = pat;

      pat++;
    } else if(pat[0] == '\\' && pat[1]) {
      pat++;
    }
  }
}

static int
bre_group_by_open(struct bre_ctx* ctx, const char* pat) {
  int i;

  for(i = 0; i < ctx->nopen; i++)
    if(ctx->open_ptr[i] == pat)
      return i;

  return -1;
}

static int
bre_group_by_close(struct bre_ctx* ctx, const char* pat) {
  int i;

  for(i = 0; i < ctx->nopen; i++)
    if(ctx->close_ptr[i] == pat)
      return i;

  return -1;
}

/* POSIX 2.13.1 bracket character class, e.g. the "lower" in
 * "[:lower:]" -- collating symbols ("[.x.]") and equivalence classes
 * ("[=x=]") are not implemented; a bracket expression containing one
 * degrades to matching nothing for that member (same policy
 * lib/path/path_fnmatch.c used before it grew real support -- not
 * duplicated here since "expr" patterns using those are vanishingly
 * rare in practice, unlike glob patterns).
 * ----------------------------------------------------------------------- */
static int
bre_class_match(const char* name, size_t namelen, int c) {
#define CLASS(s, fn) \
  if(namelen == sizeof(s) - 1 && !byte_diff(name, namelen, s)) \
  return !!fn((unsigned char)c)
  CLASS("upper", isupper);
  CLASS("lower", islower);
  CLASS("alpha", isalpha);
  CLASS("digit", isdigit);
  CLASS("alnum", isalnum);
  CLASS("punct", ispunct);
  CLASS("graph", isgraph);
  CLASS("print", isprint);
  CLASS("cntrl", iscntrl);
  CLASS("blank", isblank);
  CLASS("space", isspace);
  CLASS("xdigit", isxdigit);
#undef CLASS
  return 0;
}

/* length, in pattern bytes, of the atom starting at "pat" -- a plain
 * literal (1), "." (1), "\X" (2), or a full "[...]" bracket
 * expression. An unterminated "[" falls back to length 1 (a literal
 * "["), same policy as path_fnmatch.c's glob matcher.
 * ----------------------------------------------------------------------- */
static size_t
bre_atom_len(const char* pat) {
  if(pat[0] == '\\')
    return pat[1] ? 2 : 1;

  if(pat[0] == '[') {
    const char* p = pat + 1;

    if(*p == '^')
      p++;

    if(*p == ']')
      p++;

    while(*p && *p != ']') {
      if(p[0] == '[' && (p[1] == ':' || p[1] == '.' || p[1] == '=')) {
        char delim = p[1];
        const char* q = p + 2;

        while(*q && !(q[0] == delim && q[1] == ']'))
          q++;

        if(q[0] == delim && q[1] == ']') {
          p = q + 2;
          continue;
        }
      }

      p++;
    }

    if(*p == ']')
      return (size_t)(p + 1 - pat);
  }

  return 1;
}

static int
bre_bracket_matches(const char* pat, size_t alen, int c) {
  const char* p = pat + 1;
  const char* end = pat + alen - 1; /* points at the closing ']' */
  int neg = 0;
  int matched = 0;

  if(*p == '^') {
    neg = 1;
    p++;
  }

  while(p < end) {
    if(p[0] == '[' && p + 1 < end && (p[1] == ':' || p[1] == '.' || p[1] == '=')) {
      char delim = p[1];
      const char* q = p + 2;

      while(q < end && !(q[0] == delim && q[1] == ']'))
        q++;

      if(q < end && q[0] == delim && q[1] == ']') {
        if(delim == ':') {
          if(bre_class_match(p + 2, (size_t)(q - (p + 2)), c))
            matched = 1;
        } else if((size_t)(q - (p + 2)) == 1 && p[2] == c) {
          matched = 1;
        }

        p = q + 2;
        continue;
      }
    }

    if(p + 2 < end && p[1] == '-' && p[2] != ']') {
      if((unsigned char)c >= (unsigned char)p[0] && (unsigned char)c <= (unsigned char)p[2])
        matched = 1;

      p += 3;
      continue;
    }

    if(*p == c)
      matched = 1;

    p++;
  }

  return neg ? !matched : matched;
}

static int
bre_atom_matches(const char* pat, size_t alen, int c) {
  if(pat[0] == '\\')
    return pat[1] == c;

  if(pat[0] == '.')
    return 1; /* caller only ever passes a real (non-NUL) subject char */

  if(pat[0] == '[' && alen > 1)
    return bre_bracket_matches(pat, alen, c);

  return pat[0] == c;
}

static const char* bre_match(struct bre_ctx* ctx, const char* pat, const char* s);

/* "*": try consuming the longest possible run of the preceding atom
 * first, then back off one character at a time until the rest of the
 * pattern also matches -- for a BRE (no alternation), greedy-longest-
 * first backtracking is exactly POSIX's required leftmost-longest
 * result.
 * ----------------------------------------------------------------------- */
static const char*
bre_match_star(struct bre_ctx* ctx, const char* atom, size_t alen, const char* rest, const char* s) {
  size_t maxrun = 0;
  size_t k;

  while(s[maxrun] && bre_atom_matches(atom, alen, (unsigned char)s[maxrun]))
    maxrun++;

  for(k = maxrun + 1; k-- > 0;) {
    const char* r = bre_match(ctx, rest, s + k);

    if(r)
      return r;
  }

  return NULL;
}

static const char*
bre_match(struct bre_ctx* ctx, const char* pat, const char* s) {
  if(*pat == 0)
    return s;

  if(pat[0] == '\\' && pat[1] == '(') {
    int idx = bre_group_by_open(ctx, pat);

    if(idx >= 0)
      ctx->gstart[idx] = s;

    return bre_match(ctx, pat + 2, s);
  }

  if(pat[0] == '\\' && pat[1] == ')') {
    int idx = bre_group_by_close(ctx, pat);

    if(idx >= 0)
      ctx->gend[idx] = s;

    return bre_match(ctx, pat + 2, s);
  }

  if(pat[0] == '^' && pat == ctx->pat_start)
    return bre_match(ctx, pat + 1, s);

  if(pat[0] == '$' && pat[1] == 0)
    return *s == 0 ? s : NULL;

  {
    size_t alen = bre_atom_len(pat);

    if(pat[alen] == '*')
      return bre_match_star(ctx, pat, alen, pat + alen + 1, s);

    if(*s == 0 || !bre_atom_matches(pat, alen, (unsigned char)*s))
      return NULL;

    return bre_match(ctx, pat + alen, s + 1);
  }
}

/* run "str : pat", filling *out with what POSIX says to print (the
 * first capture group's text, or the match length if the pattern has
 * no group; the empty string or "0" on no match, respectively) --
 * returns the exit status expr itself should report for this result
 * (0 if it's neither null nor "0", 1 otherwise).
 * ----------------------------------------------------------------------- */
static int
expr_match(const char* str, const char* pat, stralloc* out) {
  struct bre_ctx ctx;
  const char* end;
  int i;

  ctx.pat_start = pat;
  bre_prepass(&ctx, pat);

  for(i = 0; i < BRE_MAXGROUPS; i++) {
    ctx.gstart[i] = NULL;
    ctx.gend[i] = NULL;
  }

  end = bre_match(&ctx, pat, str);

  stralloc_init(out);

  if(!end) {
    if(ctx.nopen == 0)
      stralloc_catc(out, '0');

    return 1;
  }

  if(ctx.nopen > 0) {
    if(ctx.gstart[0] && ctx.gend[0])
      stralloc_catb(out, ctx.gstart[0], (size_t)(ctx.gend[0] - ctx.gstart[0]));
  } else {
    char buf[FMT_ULONG];

    stralloc_catb(out, buf, fmt_ulong(buf, (size_t)(end - str)));
  }

  return out->len == 0 || (out->len == 1 && out->s[0] == '0') ? 1 : 0;
}

/* parse and expruate arguments
 * ----------------------------------------------------------------------- */
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

#ifdef DEBUG_OUTPUT
      debug_list(expr, 0);
      debug_nl_fl();
#endif /* DEBUG_OUTPUT */

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
