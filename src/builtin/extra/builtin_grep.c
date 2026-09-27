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

#if GREP_USE_SYSTEM_REGEX
#define RE_FREE(re) regfree(re)
#else
#define RE_FREE(re) dfa_free(re)
#endif

const char help_grep[] = "    Search files for patterns.\n"
                         "\n"
                         "    -E              use Extended Regular Expressions (ERE)\n"
                         "    -v              select non-matching lines\n"
                         "    -n              precede each line by its line number\n"
                         "    -q              quiet (exit 0 on match, no output)\n"
                         "    -c              print a count of matching lines instead of the lines\n"
                         "    pattern         regular expression pattern\n"
                         "    file            file to search; '-' or omitted means stdin\n";

int
builtin_grep(int argc, char* argv[]) {
  int c, ret = 1, extended = 0, invert = 0, show_lineno = 0, quiet = 0, count_only = 0;
  char* pattern = NULL;
  char* arg;
#if GREP_USE_SYSTEM_REGEX
  regex_t re;
#else
  struct dfa re = {0};
#endif

  while((c = shell_getopt(argc, argv, "Evnqc")) > 0) {
    switch(c) {
      case 'E': extended = 1; break;
      case 'v': invert = 1; break;
      case 'n': show_lineno = 1; break;
      case 'q': quiet = 1; break;
      case 'c': count_only = 1; break;
      default: builtin_invopt(argv); return 2;
    }
  }

  if(argv[shell_optind] == NULL) {
    builtin_error(argv, "no pattern given");
    return 2;
  }

  pattern = argv[shell_optind++];

#if GREP_USE_SYSTEM_REGEX
  if(regcomp(&re, pattern, extended ? REG_EXTENDED : 0) != 0) {
    builtin_error(argv, "invalid regular expression");
    return 2;
  }
#else
  if(dfa_compile(&re, pattern, str_len(pattern), extended ? DFA_ERE : 0) != DFA_OK) {
    builtin_error(argv, "invalid regular expression");
    return 2;
  }
#endif

  if(argv[shell_optind] == NULL) {
    argv[shell_optind] = "-";
    argc++;
  }

  /* argc (not counting from shell_optind on each iteration): must be
     decided once, over the whole operand list, not per file -- fixed
     at loop entry so it doesn't drop to false once only one file is
     left to process. */
  int multiple_files = (argc - shell_optind) > 1;

  while((arg = argv[shell_optind])) {
    struct filter_in in;
    char* files[2] = {arg, NULL};
    const char* line;
    int had_nl;
    ssize_t r;
    unsigned long lineno = 1;
    unsigned long matchcount = 0;

    filter_in_init(&in, argv, files, fd_in->r);

    while((r = filter_in_line(&in, &line, &had_nl)) >= 0) {
      size_t len = (size_t)r;

      if(len > 0 && line[len - 1] == '\r')
        len--;

#if GREP_USE_SYSTEM_REGEX
      char* z = alloc(len + 1);
      int matched;

      byte_copy(z, len, line);
      z[len] = '\0';
      matched = (regexec(&re, z, 0, NULL, 0) == 0);
      alloc_free(z);
#else
      int matched = dfa_test(&re, line, len);
#endif
      if(invert)
        matched = !matched;

      if(matched) {
        ret = 0;
        if(quiet) {
          filter_in_close(&in);
          RE_FREE(&re);
          return 0;
        }

        if(count_only) {
          matchcount++;
        } else {
          if(multiple_files) {
            buffer_puts(fd_out->w, arg);
            buffer_puts(fd_out->w, ":");
          }

          if(show_lineno) {
            char lbuf[32];
            size_t ln = fmt_ulong(lbuf, lineno);
            buffer_put(fd_out->w, lbuf, ln);
            buffer_puts(fd_out->w, ":");
          }

          buffer_put(fd_out->w, line, len);
          buffer_putnlflush(fd_out->w);
        }
      }

      lineno++;
    }

    filter_in_close(&in);

    if(count_only && !in.had_error) {
      char cbuf[32];
      size_t cn = fmt_ulong(cbuf, matchcount);

      if(multiple_files) {
        buffer_puts(fd_out->w, arg);
        buffer_puts(fd_out->w, ":");
      }

      buffer_put(fd_out->w, cbuf, cn);
      buffer_putnlflush(fd_out->w);
    }

    if(++shell_optind == argc)
      break;
  }

  RE_FREE(&re);
  return ret;
}

/* filter mode (TODO.md Goal 13). Scoped to the default text/dfa
 * backend -- GREP_USE_SYSTEM_REGEX gets an inert (ops == NULL)
 * grep_filter below instead of doubling every function here for
 * regex_t too. -c/-q are aggregate/short-circuit, not a 1:1 streaming
 * filter, so open() declines them (see builtin_filter.h: nothing has
 * been printed yet at that point, so declining is always safe).
 * ----------------------------------------------------------------------- */
#if !GREP_USE_SYSTEM_REGEX
struct grep_filter_ctx {
  struct filter_in in;
  struct dfa re;
  int invert, show_lineno, multiple_files, extended, aggregate, compiled;
  unsigned long lineno;
  int had_match;
  stralloc out; /* prefixes + line, when a line cannot go out as it lies */
};

/* as builtin_grep()'s own per-file loop, but pausable -- pulls lines
 * (advancing across files) until one matches (respecting -v) and hands it
 * out, with the "file:"/"N:" prefixes when asked for. A bare line that ends
 * in a newline inside the input buffer goes out in place, newline included.
 * ----------------------------------------------------------------------- */
static int
grep_filter_step(void* arg, const char** sp, size_t* np) {
  struct grep_filter_ctx* g = arg;
  const char* line;
  int had_nl;
  ssize_t r;

  while((r = filter_in_line(&g->in, &line, &had_nl)) >= 0) {
    size_t len = (size_t)r, full = len;
    int matched;

    if(g->in.newfile) {
      g->in.newfile = 0;
      g->lineno = 1;
    }

    if(len > 0 && line[len - 1] == '\r')
      len--;

    matched = (dfa_test(&g->re, line, len) != 0) != (g->invert != 0);

    if(!matched) {
      g->lineno++;
      continue;
    }

    g->had_match = 1;
    g->lineno++;

    if(!g->multiple_files && !g->show_lineno && had_nl && len == full && line != g->in.spill) {
      *sp = line;
      *np = len + 1; /* the newline is right behind it */
      return 1;
    }

    g->out.len = 0;

    if(g->multiple_files) {
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

  return 0;
}

static int
grep_filter_status(void* arg) {
  struct grep_filter_ctx* g = arg;

  if(g->in.had_error)
    return 2;

  return g->had_match ? 0 : 1;
}

static void
grep_filter_finish(void* arg) {
  struct grep_filter_ctx* g = arg;

  if(g->compiled)
    dfa_free(&g->re);

  stralloc_free(&g->out);
}

static int
grep_filter_option(void* arg, int c) {
  struct grep_filter_ctx* g = arg;

  switch(c) {
    case 'E': g->extended = 1; return 0;
    case 'v': g->invert = 1; return 0;
    case 'n': g->show_lineno = 1; return 0;
    case 'q':
    case 'c': g->aggregate = 1; return 0; /* aggregate/short-circuit, not a streaming filter */
    default: return -1;
  }
}

/* takes the pattern off the operands; anything the real invocation
 * should report (no pattern, bad pattern) declines instead */
static int
grep_filter_setup(void* arg) {
  struct grep_filter_ctx* g = arg;
  const char* pattern;

  if(g->aggregate || !g->in.files)
    return 1;

  pattern = *g->in.files++;

  if(!*g->in.files)
    g->in.files = NULL;

  if(dfa_compile(&g->re, pattern, str_len(pattern), g->extended ? DFA_ERE : 0) != DFA_OK)
    return 1;

  g->compiled = 1;
  g->multiple_files = g->in.files && g->in.files[0] && g->in.files[1];
  g->lineno = 1;
  return 0;
}

const struct filter_ops grep_ops = {
    .opts = "Evnqc",
    .size = sizeof(struct grep_filter_ctx),
    .option = grep_filter_option,
    .setup = grep_filter_setup,
    .step = grep_filter_step,
    .status = grep_filter_status,
    .finish = grep_filter_finish,
};
const struct builtin_filter grep_filter = {&grep_ops};
#else
const struct builtin_filter grep_filter = {NULL}; /* not implemented for the system regex.h backend */
#endif /* !GREP_USE_SYSTEM_REGEX */
