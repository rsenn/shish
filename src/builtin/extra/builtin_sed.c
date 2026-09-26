#include "../../builtin.h"
#include "../../fdtable.h"
#include "../../../text/sed.h"
#include "../../../lib/shell.h"
#include "../../../lib/str.h"
#include "../../../lib/byte.h"
#include "../../../lib/alloc.h"
#include "../../../lib/open.h"
#include "../../../lib/stralloc.h"

const char help_sed[] = "    Stream editor: apply a script of editing commands to each line of\n"
                        "    input, writing the result to standard output.\n"
                        "\n"
                        "    -n              suppress the default output (only 'p' prints)\n"
                        "    -e script       add script (may be given more than once)\n"
                        "    -f file         add the script read from file\n"
                        "    -E, -r          use Extended Regular Expressions (ERE)\n"
                        "    file            file(s) to edit; '-' or omitted means stdin\n";

/* slurp_file: reads the whole of path into *out (appended). Returns 0
 * on success, -1 on error (path could not be opened or read).
 * ----------------------------------------------------------------------- */
static void
slurp_sink(void* out, const char* s, size_t n) {
  stralloc_catb(out, s, n);
}

static int
slurp_file(const char* path, stralloc* out) {
  return filter_copy(path, slurp_sink, out);
}

/* w/s///w files: opened/truncated once up front, closed once at the
 * end (POSIX: "each wfile shall be created before processing
 * begins"); "/dev/stdout"/"/dev/stderr" alias the shell's own fds
 * instead of being reopened.
 * ----------------------------------------------------------------------- */
struct sed_wfile_rt {
  buffer b;
  char wbuf[512];
  int fd;
  int alias; /* 0 = own fd; 1 = fd_out->w; 2 = fd_err->w */
};

/* everything the callbacks (read/out/wfile/rfile) need, as one ctx
   shared by all of them -- text/sed.h hands the same pointer to each. */
struct sed_ctx {
  struct filter_in in;
  char linebuf[8192];

  struct sed_wfile_rt* wfiles;
  size_t nwfiles;
};

static int
sed_read_line(void* ctx, const char** sp, size_t* np, int* had_nl) {
  struct sed_ctx* c = ctx;
  ssize_t r = filter_in_get(&c->in, c->linebuf, sizeof(c->linebuf) - 1, "\n", 1);

  if(r <= 0)
    return 0;

  *had_nl = (c->linebuf[r - 1] == '\n');

  if(*had_nl)
    r--;

  *sp = c->linebuf;
  *np = (size_t)r;
  return 1;
}

static void
sed_out(void* ctx, const char* s, size_t n) {
  (void)ctx;
  buffer_put(fd_out->w, s, n);
}

static void
rfile_sink(void* ctx, const char* s, size_t n) {
  (void)ctx;
  buffer_put(fd_out->w, s, n);
}

static void
sed_rfile(void* ctx, const char* name) {
  (void)ctx;
  filter_copy(name, rfile_sink, NULL); /* POSIX: a missing r file is silently ignored */
}

static void
sed_wfile_out(void* ctx, size_t idx, const char* s, size_t n) {
  struct sed_ctx* c = ctx;
  struct sed_wfile_rt* w = &c->wfiles[idx];

  if(w->alias == 1)
    buffer_put(fd_out->w, s, n);
  else if(w->alias == 2)
    buffer_put(fd_err->w, s, n);
  else if(w->fd != -1)
    buffer_put(&w->b, s, n);
}

int
builtin_sed(int argc, char* argv[]) {
  int c, autoprint_off = 0, flags = 0, ret = 0, exit_status = 0;
  int have_script = 0;
  stralloc script;
  struct sed* prog;
  int rc;
  struct sed_ctx ctx;
  struct sed_state* st;
  size_t i;

  stralloc_init(&script);

  while((c = shell_getopt(argc, argv, "nEre:f:")) > 0) {
    switch(c) {
      case 'n': autoprint_off = 1; break;
      case 'E':
      case 'r': flags |= SED_ERE; break;

      case 'e':
        if((have_script && !stralloc_catc(&script, '\n')) ||
           !stralloc_cats(&script, shell_optarg)) {
          builtin_error(argv, "out of memory");
          stralloc_free(&script);
          return 2;
        }

        have_script = 1;
        break;

      case 'f':
        if(have_script && !stralloc_catc(&script, '\n')) {
          builtin_error(argv, "out of memory");
          stralloc_free(&script);
          return 2;
        }

        if(slurp_file(shell_optarg, &script) == -1) {
          builtin_error(argv, shell_optarg);
          stralloc_free(&script);
          return 2;
        }

        have_script = 1;
        break;

      default: builtin_invopt(argv); return 2;
    }
  }

  if(!have_script) {
    if(argv[shell_optind] == NULL) {
      builtin_error(argv, "no script given");
      stralloc_free(&script);
      return 2;
    }

    stralloc_cats(&script, argv[shell_optind++]);
  }

  if(autoprint_off)
    flags |= SED_NOAUTOPRINT;

  rc = sed_compile(&prog, script.s ? script.s : "", script.len, (unsigned)flags);
  stralloc_free(&script);

  if(rc != SED_OK) {
    builtin_errmsg(argv, "script", (char*)sed_error(rc));
    return 2;
  }

  byte_zero(&ctx, sizeof(ctx));
  filter_in_init(&ctx.in, argv, argv[shell_optind] ? argv + shell_optind : NULL, fd_in->r);
  ctx.nwfiles = sed_wfile_count(prog);

  if(ctx.nwfiles) {
    ctx.wfiles = alloc(ctx.nwfiles * sizeof(*ctx.wfiles));

    if(!ctx.wfiles) {
      builtin_error(argv, "out of memory");
      sed_free(prog);
      return 2;
    }

    for(i = 0; i < ctx.nwfiles; i++) {
      const char* name = sed_wfile_name(prog, i);

      byte_zero(&ctx.wfiles[i], sizeof(ctx.wfiles[i]));

      if(!str_diff(name, "/dev/stdout")) {
        ctx.wfiles[i].alias = 1;
        ctx.wfiles[i].fd = -1;
      } else if(!str_diff(name, "/dev/stderr")) {
        ctx.wfiles[i].alias = 2;
        ctx.wfiles[i].fd = -1;
      } else {
        ctx.wfiles[i].fd = open_trunc(name);

        if(ctx.wfiles[i].fd == -1) {
          builtin_error(argv, (char*)name);
          ret = 2;
        } else {
          buffer_init(&ctx.wfiles[i].b,
                      &buffer_op_write,
                      ctx.wfiles[i].fd,
                      ctx.wfiles[i].wbuf,
                      sizeof(ctx.wfiles[i].wbuf));
        }
      }
    }
  }

  st = sed_state_new(
      prog, sed_read_line, sed_out, ctx.nwfiles ? sed_wfile_out : NULL, sed_rfile, &ctx);

  if(!st) {
    builtin_error(argv, "out of memory");
    ret = 2;
  } else {
    sed_run(st, &exit_status);
    sed_state_free(st);
  }

  buffer_flush(fd_out->w);

  for(i = 0; i < ctx.nwfiles; i++) {
    if(ctx.wfiles[i].alias == 0 && ctx.wfiles[i].fd != -1) {
      buffer_flush(&ctx.wfiles[i].b);
      buffer_close(&ctx.wfiles[i].b);
    }
  }

  filter_in_close(&ctx.in);

  alloc_free(ctx.wfiles);
  sed_free(prog);

  if(ctx.in.had_error || ret)
    return ret ? ret : 2;

  return exit_status;
}

/* filter mode (TODO.md Goal 13). sed's own control flow (n/N, hold
 * space, branches) isn't incrementally resumable yet -- see that
 * section's open questions -- so this runs sed_run() to completion on
 * the first read() call, capturing its output into an in-memory
 * buffer that later read() calls just drain. That gives up true
 * streaming (no interleaving with whatever consumes this filter's
 * output while sed itself runs) but still avoids the fork()+pipe()
 * pair, which is the win this goal actually targets -- and it's the
 * same "run to completion, hand back the result" shape wc's own
 * filter mode uses for its aggregate output, not a special case.
 * Scoped to no file operands (only "-"/the upstream pipe) and no
 * w/s///w targets (those need real fds regardless of chaining, and
 * sed_filter_out() below only captures the pattern-space output) --
 * both just decline via open() returning NULL.
 * ----------------------------------------------------------------------- */
struct sed_filter_ctx {
  struct sed* prog;
  struct sed_state* st;
  buffer* upstream;
  stralloc out;
  size_t outpos;
  int ran;
  int exit_status;
  char linebuf[8192];
};

static int
sed_filter_read_line(void* ctx, const char** sp, size_t* np, int* had_nl) {
  struct sed_filter_ctx* c = ctx;
  int r = buffer_get_until(c->upstream, c->linebuf, sizeof(c->linebuf) - 1, "\n", 1);

  if(r <= 0)
    return 0;

  *had_nl = (c->linebuf[r - 1] == '\n');

  if(*had_nl)
    r--;

  *sp = c->linebuf;
  *np = (size_t)r;
  return 1;
}

static void
sed_filter_out(void* ctx, const char* s, size_t n) {
  struct sed_filter_ctx* c = ctx;

  stralloc_catb(&c->out, s, n);
}

static void
sed_filter_rfile(void* ctx, const char* name) {
  struct sed_filter_ctx* c = ctx;

  filter_copy(name, slurp_sink, &c->out); /* POSIX: a missing r file is silently ignored */
}

static ssize_t
sed_filter_read(int fd, void* buf, size_t len, void* arg) {
  struct sed_filter_ctx* c = arg;
  size_t take;

  (void)fd;

  if(!c->ran) {
    c->ran = 1;
    sed_run(c->st, &c->exit_status);
    sed_state_free(c->st);
    c->st = NULL;
  }

  take = c->out.len - c->outpos;

  if(take > len)
    take = len;

  if(take)
    byte_copy(buf, take, c->out.s + c->outpos);

  c->outpos += take;
  return (ssize_t)take;
}

static int
sed_filter_status(void* arg) {
  struct sed_filter_ctx* c = arg;

  return c->exit_status;
}

static void
sed_filter_close(void* arg) {
  struct sed_filter_ctx* c = arg;

  if(c->st)
    sed_state_free(c->st);

  stralloc_free(&c->out);
  sed_free(c->prog);
  alloc_free(c);
}

static void*
sed_filter_open(int argc, char* argv[], buffer* upstream) {
  int c, autoprint_off = 0, have_script = 0;
  unsigned flags = 0;
  stralloc script;
  struct sed* prog;
  int rc;
  struct sed_filter_ctx* fc;

  stralloc_init(&script);

  while((c = shell_getopt(argc, argv, "nEre:f:")) > 0) {
    switch(c) {
      case 'n': autoprint_off = 1; break;
      case 'E':
      case 'r': flags |= SED_ERE; break;

      case 'e':
        if((have_script && !stralloc_catc(&script, '\n')) ||
           !stralloc_cats(&script, shell_optarg)) {
          stralloc_free(&script);
          return NULL;
        }

        have_script = 1;
        break;

      case 'f':
        if(have_script && !stralloc_catc(&script, '\n')) {
          stralloc_free(&script);
          return NULL;
        }

        if(slurp_file(shell_optarg, &script) == -1) {
          stralloc_free(&script);
          return NULL;
        }

        have_script = 1;
        break;

      /* bad option: return NULL *without* printing -- see
         cat_filter_open()'s identical comment in builtin_cat.c */
      default: stralloc_free(&script); return NULL;
    }
  }

  if(!have_script) {
    if(argv[shell_optind] == NULL) {
      stralloc_free(&script);
      return NULL; /* "no script given": let the real invocation report it */
    }

    stralloc_cats(&script, argv[shell_optind++]);
  }

  if(argv[shell_optind] != NULL) {
    stralloc_free(&script);
    return NULL; /* file operands: filter mode only handles the piped/stdin case */
  }

  if(autoprint_off)
    flags |= SED_NOAUTOPRINT;

  rc = sed_compile(&prog, script.s ? script.s : "", script.len, (unsigned)flags);
  stralloc_free(&script);

  if(rc != SED_OK)
    return NULL; /* bad script: nothing printed yet, let the real invocation report it */

  if(sed_wfile_count(prog)) {
    sed_free(prog);
    return NULL; /* w/s///w targets need real fds regardless of chaining */
  }

  if(!(fc = alloc(sizeof(*fc)))) {
    sed_free(prog);
    return NULL;
  }

  byte_zero(fc, sizeof(*fc));
  fc->prog = prog;
  fc->upstream = upstream;
  stralloc_init(&fc->out);

  fc->st = sed_state_new(prog, sed_filter_read_line, sed_filter_out, NULL, sed_filter_rfile, fc);

  if(!fc->st) {
    sed_free(prog);
    alloc_free(fc);
    return NULL;
  }

  return fc;
}

const struct filter_ops sed_ops = {sed_filter_open,
                                   sed_filter_read,
                                   sed_filter_status,
                                   sed_filter_close};
const struct builtin_filter sed_filter = {&sed_ops};
