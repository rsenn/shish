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

#if GREP_USE_SYSTEM_REGEX
#define RE_FREE(re) regfree(re)
#else
#define RE_FREE(re) dfa_free(re)
#endif

const char help_grep[] =
    "    Search files for patterns.\n"
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

  while((arg = argv[shell_optind])) {
    char buf[1024], rbuf[1024];
    buffer inb, *in;
    ssize_t r;
    unsigned long lineno = 1;
    unsigned long matchcount = 0;
    /* argc (not argv[shell_optind+1]): when no file operand was
       given, argv[shell_optind] was just set to "-" in place of the
       NULL terminator, so the slot after it is past the real array
       and unsafe to read. */
    int multiple_files = (shell_optind + 1 < argc);

    if(!str_diff(arg, "-")) {
      in = fd_in->r;
    } else {
      in = &inb;
      if(buffer_mmapread(in, arg)) {
        int rfd = open_read(arg);
        if(rfd == -1) {
          builtin_error(argv, arg);
          goto next_file;
        }
        buffer_init(in, &buffer_op_read, rfd, rbuf, sizeof(rbuf));
      }
    }

    for(;;) {
      if((r = buffer_get_until(in, buf, sizeof(buf) - 1, "\n", 1)) <= 0)
        break;

      buf[r] = '\0';
      if(r > 0 && buf[r - 1] == '\n')
        buf[--r] = '\0';
      if(r > 0 && buf[r - 1] == '\r')
        buf[--r] = '\0';

#if GREP_USE_SYSTEM_REGEX
      int matched = (regexec(&re, buf, 0, NULL, 0) == 0);
#else
      int matched = dfa_test(&re, buf, (size_t)r);
#endif
      if(invert)
        matched = !matched;

      if(matched) {
        ret = 0;
        if(quiet) {
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

          buffer_puts(fd_out->w, buf);
          buffer_putnlflush(fd_out->w);
        }
      }

      lineno++;
    }

    if(count_only) {
      char cbuf[32];
      size_t cn = fmt_ulong(cbuf, matchcount);

      if(multiple_files) {
        buffer_puts(fd_out->w, arg);
        buffer_puts(fd_out->w, ":");
      }

      buffer_put(fd_out->w, cbuf, cn);
      buffer_putnlflush(fd_out->w);
    }

  next_file:
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
  struct dfa re;
  int invert, show_lineno, multiple_files;
  char** files; /* argv+shell_optind, or NULL: only "-"/upstream */
  int i;
  int done_any;
  buffer* upstream;
  buffer inb;
  char rbuf[1024];
  buffer* cur; /* NULL: need to open the next file */
  unsigned long lineno;
  int had_match;
  int had_error;
  char* const* errargv;

  char raw[1024];
  char linebuf[1200];
  char pend[1400];
  size_t pend_off, pend_len;
};

static int
grep_filter_open_file(struct grep_filter_ctx* g) {
  for(;;) {
    const char* name;

    if(g->cur && g->cur != g->upstream)
      buffer_close(g->cur);

    g->cur = NULL;

    if(g->files) {
      name = g->files[g->i];

      if(!name)
        return 0;

      g->i++;
    } else {
      if(g->done_any)
        return 0;

      name = "-";
    }

    g->done_any = 1;
    g->lineno = 1;

    if(!str_diff(name, "-")) {
      g->cur = g->upstream;
      return 1;
    }

    g->cur = &g->inb;

    if(buffer_mmapread(g->cur, name) == 0)
      return 1;

    {
      int rfd = open_read(name);

      if(rfd == -1) {
        builtin_error((char**)g->errargv, (char*)name);
        g->had_error = 1;
        g->cur = NULL;
        continue;
      }

      buffer_init(g->cur, &buffer_op_read, rfd, g->rbuf, sizeof(g->rbuf));
      return 1;
    }
  }
}

/* grep_filter_step: as builtin_grep()'s own per-file loop, but
 * pausable -- pulls lines (advancing across files) until one matches
 * (respecting -v), formats it (with the "file:"/"N:" prefixes) into
 * g->linebuf, or returns 0 once every file is exhausted.
 * ----------------------------------------------------------------------- */
static int
grep_filter_step(struct grep_filter_ctx* g, const char** sp, size_t* np) {
  for(;;) {
    ssize_t r;
    int matched;
    size_t pos;

    if(!g->cur && !grep_filter_open_file(g))
      return 0;

    r = buffer_get_until(g->cur, g->raw, sizeof(g->raw) - 1, "\n", 1);

    if(r < 0) {
      g->had_error = 1;
      g->cur = NULL;
      continue;
    }

    if(r == 0) {
      g->cur = NULL;
      continue;
    }

    if(g->raw[r - 1] == '\n')
      r--;

    if(r > 0 && g->raw[r - 1] == '\r')
      r--;

    matched = dfa_test(&g->re, g->raw, (size_t)r);

    if(g->invert)
      matched = !matched;

    if(!matched) {
      g->lineno++;
      continue;
    }

    g->had_match = 1;
    pos = 0;

    if(g->multiple_files) {
      const char* name = g->files[g->i - 1];
      size_t nl = str_len(name);

      byte_copy(g->linebuf + pos, nl, name);
      pos += nl;
      g->linebuf[pos++] = ':';
    }

    if(g->show_lineno) {
      char lbuf[FMT_ULONG];
      ssize_t ln = fmt_ulong(lbuf, g->lineno);

      byte_copy(g->linebuf + pos, (size_t)ln, lbuf);
      pos += (size_t)ln;
      g->linebuf[pos++] = ':';
    }

    byte_copy(g->linebuf + pos, (size_t)r, g->raw);
    pos += (size_t)r;
    g->linebuf[pos++] = '\n';

    g->lineno++;
    *sp = g->linebuf;
    *np = pos;
    return 1;
  }
}

static ssize_t
grep_filter_read(int fd, void* buf, size_t len, void* arg) {
  struct grep_filter_ctx* g = arg;
  char* out = buf;
  size_t n = 0;

  (void)fd;

  if(g->pend_len) {
    size_t take = g->pend_len < (len - n) ? g->pend_len : (len - n);

    byte_copy(out + n, take, g->pend + g->pend_off);
    g->pend_off += take;
    g->pend_len -= take;
    n += take;
  }

  while(n < len) {
    const char* s;
    size_t sn;

    if(!grep_filter_step(g, &s, &sn))
      break;

    if(sn <= len - n) {
      byte_copy(out + n, sn, s);
      n += sn;
    } else {
      size_t take = len - n;

      byte_copy(out + n, take, s);
      n += take;

      g->pend_len = sn - take;
      byte_copy(g->pend, g->pend_len, s + take);
      g->pend_off = 0;
      break;
    }
  }

  return (ssize_t)n;
}

static int
grep_filter_status(void* arg) {
  struct grep_filter_ctx* g = arg;

  if(g->had_error)
    return 2;

  return g->had_match ? 0 : 1;
}

static void
grep_filter_close(void* arg) {
  struct grep_filter_ctx* g = arg;

  if(g->cur && g->cur != g->upstream)
    buffer_close(g->cur);

  dfa_free(&g->re);
  alloc_free(g);
}

static void*
grep_filter_open(int argc, char* argv[], buffer* upstream) {
  int c, extended = 0, invert = 0, show_lineno = 0, quiet = 0, count_only = 0;
  char* pattern;
  struct grep_filter_ctx* g;

  while((c = shell_getopt(argc, argv, "Evnqc")) > 0) {
    switch(c) {
      case 'E': extended = 1; break;
      case 'v': invert = 1; break;
      case 'n': show_lineno = 1; break;
      case 'q': quiet = 1; break;
      case 'c': count_only = 1; break;
      default: return NULL; /* bad option: see cat_filter_open()'s identical comment */
    }
  }

  if(quiet || count_only)
    return NULL; /* aggregate/short-circuit, not a streaming filter */

  if(argv[shell_optind] == NULL)
    return NULL; /* "no pattern given": let the real invocation report it */

  pattern = argv[shell_optind++];

  if(!(g = alloc(sizeof(*g))))
    return NULL;

  byte_zero(g, sizeof(*g));
  g->upstream = upstream;
  g->invert = invert;
  g->show_lineno = show_lineno;
  g->errargv = argv;

  if(dfa_compile(&g->re, pattern, str_len(pattern), extended ? DFA_ERE : 0) != DFA_OK) {
    alloc_free(g); /* bad pattern: nothing printed yet, let the real invocation report it */
    return NULL;
  }

  g->files = (argv[shell_optind] != NULL) ? argv + shell_optind : NULL;
  g->multiple_files = g->files && g->files[0] && g->files[1];
  g->lineno = 1;
  return g;
}

const struct filter_ops grep_ops = {grep_filter_open,
                                    grep_filter_read,
                                    grep_filter_status,
                                    grep_filter_close};
const struct builtin_filter grep_filter = {&grep_ops};
#else
const struct builtin_filter grep_filter = {
    NULL}; /* not implemented for the system regex.h backend */
#endif /* !GREP_USE_SYSTEM_REGEX */
