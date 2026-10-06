#include "builtin_config.h"

#if BUILTIN_GREP

#if GREP_USE_SYSTEM_REGEX
#include <regex.h>
#else
#include "../../../text/dfa.h"
#endif
#include "../../builtin.h"
#include "../../fdtable.h"
#include "../../term.h"
#include "../../../lib/fmt.h"
#include "../../../lib/shell.h"
#include "../../../lib/str.h"
#include "../../../lib/open.h"
#include "../../../lib/byte.h"
#include "../../../lib/alloc.h"
#include "../../../lib/stralloc.h"

const char help_grep[] = "    Search files for patterns.\n"
                         "\n"
                         "    -E              use Extended Regular Expressions (ERE)\n"
                         "    -F              match fixed strings (newline separates several)\n"
                         "    -i              ignore case (ASCII)\n"
                         "    -v              select non-matching lines\n"
                         "    -x              select only lines that match as a whole\n"
                         "    -n              precede each line by its line number\n"
                         "    -e pattern      pattern to search for; may be given several times\n"
                         "    -f file         read patterns from file, one per line\n"
                         "    -l              print only the names of files with a match\n"
                         "    -s              no messages about files that cannot be opened\n"
                         "    -q              quiet (exit 0 on match, no output)\n"
                         "    -c              print a count of matching lines instead of the lines\n"
                         "    pattern         regular expression pattern\n"
                         "    file            file to search; '-' or omitted means stdin\n";

/* one backend behind three names: compile, test one line, free */
#if GREP_USE_SYSTEM_REGEX
typedef regex_t grep_re;
#define RE_FREE(re) regfree(re)
#else
typedef struct dfa grep_re;
#define RE_FREE(re) dfa_free(re)
#endif

struct grep {
  struct filter_in in;
  grep_re re;
  unsigned invert : 1, show_lineno : 1, extended : 1, fixed : 1, quiet : 1, count : 1, multiple : 1, compiled : 1,
      had_match : 1, done : 1, pending : 1, whole : 1, icase : 1, list : 1, nomsg : 1, haspat : 1;
  unsigned long lineno, matches;
  const char* fixed_pat; /* -F: the pattern operand, '\n'-separated strings */
  char* joined_pat;      /* several -e patterns as one regular expression */
  char* whole_pat;       /* -x: the pattern wrapped in ^( )$ */
  const char* name;      /* -c: the operand the count in progress belongs to */
  stralloc epat;         /* -e: the patterns, '\n'-separated */
  stralloc out;          /* prefixes + line, when a line cannot go out as it lies */
};

/* -i: byte_equal() ignoring ASCII case */
static int
grep_equal_i(const char* a, const char* b, size_t n) {
  for(; n; n--, a++, b++) {
    int x = (unsigned char)*a, y = (unsigned char)*b;

    if(x >= 'A' && x <= 'Z')
      x += 'a' - 'A';

    if(y >= 'A' && y <= 'Z')
      y += 'a' - 'A';

    if(x != y)
      return 0;
  }

  return 1;
}

/* -f: appends the file's bytes to the -e patterns */
static void
grep_pat_sink(void* ctx, const char* p, size_t n) {
  stralloc_catb(&((struct grep*)ctx)->epat, p, n);
}

/* -F: does any '\n'-separated string of the pattern occur in line? "" matches everything */
static int
grep_test_fixed(const char* pat, const char* line, size_t len, int whole, int icase) {
  for(;;) {
    size_t n = str_chr(pat, '\n'), i;

    if(whole) {
      if(n == len && (icase ? grep_equal_i(line, pat, n) : byte_equal(line, n, pat)))
        return 1;
    } else if(n <= len)
      for(i = 0; i + n <= len; i++)
        if(icase ? grep_equal_i(line + i, pat, n) : byte_equal(line + i, n, pat))
          return 1;

    if(!pat[n])
      return 0;

    pat += n + 1;
  }
}

static int
grep_test(struct grep* g, const char* line, size_t len) {
  if(g->haspat && !g->epat.len)
    return 0; /* -f with an empty file: no pattern, no match */

  if(g->fixed)
    return grep_test_fixed(g->fixed_pat, line, len, g->whole, g->icase);

#if GREP_USE_SYSTEM_REGEX
  char* z = alloc(len + 1);
  int m;

  byte_copy(z, len, line);
  z[len] = '\0';
  m = regexec(&g->re, z, 0, NULL, 0) == 0;
  alloc_free(z);
  return m;
#else
  return dfa_test(&g->re, line, len) != 0;
#endif
}

/* -c: "count" or "file:count" for the operand just finished */
static void
grep_count_line(struct grep* g) {
  char lbuf[FMT_ULONG];

  g->out.len = 0;

  if(g->multiple) {
    stralloc_cats(&g->out, g->name);
    stralloc_catc(&g->out, ':');
  }

  stralloc_catb(&g->out, lbuf, fmt_ulong(lbuf, g->matches));
  stralloc_catc(&g->out, '\n');
  g->matches = 0;
  g->pending = 0;
}

/* is the line selected (-v applied)? */
static int
grep_select(struct grep* g, const char* line, size_t len) {
  return grep_test(g, line, len) != (g->invert != 0);
}

/* pulls lines (across operands) until one is selected and hands it out, with the
 * "file:"/"N:" prefixes when asked for. A bare line that ends in a newline inside
 * the input buffer goes out in place, newline included. -c yields one count per
 * operand (out when the next operand's first line arrives, or at the end); -q
 * stops at the first match. */
static int
grep_step(void* arg, const char** sp, size_t* np) {
  struct grep* g = arg;
  const char* line;
  int had_nl;
  ssize_t r;

  while(!g->done && (r = filter_in_line(&g->in, &line, &had_nl)) >= 0) {
    size_t len = (size_t)r, full = len;
    int due = 0;

    if(g->in.newfile) {
      g->in.newfile = 0;

      if(g->count && g->pending) {
        grep_count_line(g);
        due = 1;
      }

      g->lineno = 1;
      g->name = filter_in_name(&g->in);
      g->pending = 1;
    }

    if(len > 0 && line[len - 1] == '\r')
      len--;

    g->lineno++;

    if(grep_select(g, line, len)) {
      g->had_match = 1;

      if(g->list && !g->quiet) {
        g->out.len = 0;
        stralloc_cats(&g->out, filter_in_name(&g->in)[0] == '-' && !filter_in_name(&g->in)[1] ? "(standard input)" : filter_in_name(&g->in));
        stralloc_catc(&g->out, '\n');
        filter_in_close(&g->in); /* one name per file: on to the next */
        *sp = g->out.s;
        *np = g->out.len;
        return 1;
      }

      if(g->quiet) {
        g->done = 1;
        return 0;
      }

      if(g->count)
        g->matches++;
      else {
        if(!g->multiple && !g->show_lineno && had_nl && len == full && line != g->in.spill) {
          *sp = line;
          *np = len + 1; /* the newline is right behind it */
          return 1;
        }

        g->out.len = 0;

        if(g->multiple) {
          stralloc_cats(&g->out, filter_in_name(&g->in));
          stralloc_catc(&g->out, ':');
        }

        if(g->show_lineno) {
          char lbuf[FMT_ULONG];

          stralloc_catb(&g->out, lbuf, fmt_ulong(lbuf, g->lineno - 1));
          stralloc_catc(&g->out, ':');
        }

        stralloc_catb(&g->out, line, len);
        stralloc_catc(&g->out, '\n');
        *sp = g->out.s;
        *np = g->out.len;
        return 1;
      }
    }

    if(due) {
      *sp = g->out.s;
      *np = g->out.len;
      return 1;
    }
  }

  /* end of input: -c owes the last operand's count */
  if(g->count && g->pending && !g->quiet) {
    grep_count_line(g);
    *sp = g->out.s;
    *np = g->out.len;
    return 1;
  }

  return 0;
}

static int
grep_option(void* ctx, int c) {
  struct grep* g = ctx;

  switch(c) {
    case 'E': g->extended = 1; return 0;
    case 'F': g->fixed = 1; return 0;
    case 'e':
      g->haspat = 1;

      if(g->epat.len)
        stralloc_catc(&g->epat, '\n');

      stralloc_cats(&g->epat, shell_optarg);
      return 0;

    case 'f': {
      size_t before = g->epat.len;

      if(before)
        stralloc_catc(&g->epat, '\n');

      if(filter_copy(shell_optarg, grep_pat_sink, g) < 0) {
        g->in.err_arg = shell_optarg;
        g->in.err_msg = "cannot read pattern file";
        return -1;
      }

      /* a trailing newline ends the last pattern, it does not start another */
      if(g->epat.len > before && g->epat.s[g->epat.len - 1] == '\n')
        g->epat.len--;
      else if(g->epat.len == before + 1 && before)
        g->epat.len--; /* empty file after other patterns: drop the separator */

      g->haspat = 1;
      return 0;
    }

    case 'l': g->list = 1; return 0;
    case 's': g->nomsg = 1; return 0;
    case 'i': g->icase = 1; return 0;
    case 'v': g->invert = 1; return 0;
    case 'x': g->whole = 1; return 0;
    case 'n': g->show_lineno = 1; return 0;
    case 'q': g->quiet = 1; return 0;
    case 'c': g->count = 1; return 0;
    default: return -1;
  }
}

/* takes the pattern off the operands and compiles it */
static int
grep_setup(void* ctx) {
  struct grep* g = ctx;
  const char* pattern;

  g->in.silent = g->nomsg; /* filter_in_init() cleared it after option() */

  if(g->haspat && !g->epat.len) {
    g->multiple = g->in.files && g->in.files[0] && g->in.files[1];
    g->lineno = 1;
    return 0;
  }

  if(g->haspat) {
    stralloc_nul(&g->epat);
    pattern = g->epat.s;
  } else {
    if(!g->in.files) {
      g->in.err_arg = "";
      g->in.err_msg = "no pattern given";
      return -1;
    }

    pattern = *g->in.files++;

    if(!*g->in.files)
      g->in.files = NULL;
  }

  g->multiple = g->in.files && g->in.files[0] && g->in.files[1];
  g->lineno = 1;

  if(g->fixed) {
    g->fixed_pat = pattern;
    return 0;
  }

  /* several -e patterns: "a\nb" -> "a\|b" (BRE) or "a|b" (ERE) */
  if(str_chr(pattern, '\n')[pattern]) {
    stralloc j;
    size_t n;

    stralloc_init(&j);

    for(;;) {
      n = str_chr(pattern, '\n');
      stralloc_catb(&j, pattern, n);

      if(!pattern[n])
        break;

      stralloc_cats(&j, g->extended ? "|" : "\\|");
      pattern += n + 1;
    }

    stralloc_nul(&j);
    g->joined_pat = j.s;
    pattern = g->joined_pat;
  }

  /* -x: "^(pattern)$" -- "\\(" in a basic regular expression */
  if(g->whole) {
    stralloc w;

    stralloc_init(&w);
    stralloc_cats(&w, g->extended ? "^(" : "^\\(");
    stralloc_cats(&w, pattern);
    stralloc_cats(&w, g->extended ? ")$" : "\\)$");
    stralloc_nul(&w);
    g->whole_pat = w.s;
    pattern = g->whole_pat;
  }

#if GREP_USE_SYSTEM_REGEX
  if(regcomp(&g->re, pattern, (g->extended ? REG_EXTENDED : 0) | (g->icase ? REG_ICASE : 0)) != 0) {
#else
  if(dfa_compile(&g->re, pattern, str_len(pattern), (g->extended ? DFA_ERE : 0) | (g->icase ? DFA_ICASE : 0)) != DFA_OK) {
#endif
    g->in.err_arg = "";
    g->in.err_msg = "invalid regular expression";
    return -1;
  }

  g->compiled = 1;
  return 0;
}

static int
grep_status(void* ctx) {
  struct grep* g = ctx;

  /* an unreadable operand does not change 0/1 (tests/builtin-grep.sh) */
  return g->had_match ? 0 : 1;
}

static void
grep_finish(void* ctx) {
  struct grep* g = ctx;

  if(g->compiled)
    RE_FREE(&g->re);

  alloc_free(g->whole_pat);
  alloc_free(g->joined_pat);
  stralloc_free(&g->epat);

  stralloc_free(&g->out);
}

const struct filter_ops grep_ops = {
    .opts = "EFe:f:ilsvxnqc",
    .size = sizeof(struct grep),
    .option = grep_option,
    .setup = grep_setup,
    .step = grep_step,
    .status = grep_status,
    .finish = grep_finish,
    .err_status = 2,
};

FILTER_BUILTIN(grep)
#endif /* BUILTIN_GREP */
