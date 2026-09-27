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
        if((have_script && !stralloc_catc(&script, '\n')) || !stralloc_cats(&script, shell_optarg)) {
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
          buffer_init(
              &ctx.wfiles[i].b, &buffer_op_write, ctx.wfiles[i].fd, ctx.wfiles[i].wbuf, sizeof(ctx.wfiles[i].wbuf));
        }
      }
    }
  }

  st = sed_state_new(prog, sed_read_line, sed_out, ctx.nwfiles ? sed_wfile_out : NULL, sed_rfile, &ctx);

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
 * the first step() call, capturing its output into an in-memory
 * buffer that later step() calls hand back. That gives up true
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
  struct filter_in in;
  stralloc script, out;
  unsigned flags;
  int have_script, autoprint_off, ran, sent, exit_status;
  struct sed* prog;
  struct sed_state* st;
  char linebuf[8192];
};

static int
sed_filter_read_line(void* ctx, const char** sp, size_t* np, int* had_nl) {
  struct sed_filter_ctx* c = ctx;
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
sed_filter_out(void* ctx, const char* s, size_t n) {
  struct sed_filter_ctx* c = ctx;

  stralloc_catb(&c->out, s, n);
}

static void
sed_filter_rfile(void* ctx, const char* name) {
  struct sed_filter_ctx* c = ctx;

  filter_copy(name, slurp_sink, &c->out); /* POSIX: a missing r file is silently ignored */
}

/* runs the script to completion on the first call, then hands out the
 * captured output as a single unit */
static int
sed_filter_step(void* arg, const char** unit, size_t* len) {
  struct sed_filter_ctx* c = arg;

  if(!c->ran) {
    c->ran = 1;
    sed_run(c->st, &c->exit_status);
    sed_state_free(c->st);
    c->st = NULL;
  }

  if(c->sent || !c->out.len)
    return 0;

  c->sent = 1;
  *unit = c->out.s;
  *len = c->out.len;
  return 1;
}

static int
sed_filter_status(void* arg) {
  struct sed_filter_ctx* c = arg;

  return c->exit_status;
}

static void
sed_filter_finish(void* arg) {
  struct sed_filter_ctx* c = arg;

  if(c->st)
    sed_state_free(c->st);

  if(c->prog)
    sed_free(c->prog);

  stralloc_free(&c->script);
  stralloc_free(&c->out);
}

static int
sed_filter_option(void* arg, int ch) {
  struct sed_filter_ctx* c = arg;

  switch(ch) {
    case 'n': c->autoprint_off = 1; return 0;
    case 'E':
    case 'r': c->flags |= SED_ERE; return 0;

    case 'e':
      if((c->have_script && !stralloc_catc(&c->script, '\n')) || !stralloc_cats(&c->script, shell_optarg))
        return -1;

      c->have_script = 1;
      return 0;

    case 'f':
      if(c->have_script && !stralloc_catc(&c->script, '\n'))
        return -1;

      if(slurp_file(shell_optarg, &c->script) == -1)
        return -1;

      c->have_script = 1;
      return 0;
  }

  return -1;
}

/* everything the real invocation should report (no script, bad script)
 * or cannot stream (file operands, w/s///w targets needing real fds) declines */
static int
sed_filter_setup(void* arg) {
  struct sed_filter_ctx* c = arg;

  if(!c->have_script) {
    if(!c->in.files)
      return 1;

    stralloc_cats(&c->script, *c->in.files++);

    if(!*c->in.files)
      c->in.files = NULL;
  }

  if(c->in.files)
    return 1;

  if(c->autoprint_off)
    c->flags |= SED_NOAUTOPRINT;

  if(sed_compile(&c->prog, c->script.s ? c->script.s : "", c->script.len, c->flags) != SED_OK) {
    c->prog = NULL;
    return 1;
  }

  if(sed_wfile_count(c->prog))
    return 1;

  return (c->st = sed_state_new(c->prog, sed_filter_read_line, sed_filter_out, NULL, sed_filter_rfile, c)) ? 0 : 1;
}

const struct filter_ops sed_ops = {
    .opts = "nEre:f:",
    .size = sizeof(struct sed_filter_ctx),
    .option = sed_filter_option,
    .setup = sed_filter_setup,
    .step = sed_filter_step,
    .status = sed_filter_status,
    .finish = sed_filter_finish,
};
const struct builtin_filter sed_filter = {&sed_ops};
