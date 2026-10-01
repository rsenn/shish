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
  stralloc script, out; /* out: the result, when there is no sink (a chain) */
  unsigned flags;
  unsigned have_script : 1, autoprint_off : 1, ran : 1, sent : 1;
  int exit_status, ret;
  struct sed* prog;
  struct sed_state* st;
  struct sed_wfile_rt* wfiles;
  size_t nwfiles;
};

static int
sed_read_line(void* ctx, const char** sp, size_t* np, int* had_nl) {
  struct sed_ctx* c = ctx;
  ssize_t r = filter_in_line(&c->in, sp, had_nl);

  if(r < 0)
    return 0;

  *np = (size_t)r;
  return 1;
}

/* the result: straight to the sink, or collected for the chain */
static void
sed_out(void* ctx, const char* s, size_t n) {
  struct sed_ctx* c = ctx;

  if(c->in.sink)
    buffer_put(c->in.sink, s, n);
  else
    stralloc_catb(&c->out, s, n);
}

static void
sed_sink(void* out, const char* s, size_t n) {
  buffer_put(out, s, n);
}

static void
sed_rfile(void* ctx, const char* name) {
  struct sed_ctx* c = ctx;

  if(c->in.sink)
    filter_copy(name, sed_sink, c->in.sink); /* POSIX: a missing r file is silently ignored */
  else
    filter_copy(name, slurp_sink, &c->out);
}

static void
sed_wfile_out(void* ctx, size_t idx, const char* s, size_t n) {
  struct sed_ctx* c = ctx;
  struct sed_wfile_rt* w = &c->wfiles[idx];

  if(w->alias == 1)
    sed_out(c, s, n);
  else if(w->alias == 2)
    buffer_put(fd_err->w, s, n);
  else if(w->fd != -1)
    buffer_put(&w->b, s, n);
}

static int
sed_option(void* ctx, int ch) {
  struct sed_ctx* c = ctx;

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

      if(slurp_file(shell_optarg, &c->script) == -1) {
        c->in.err_arg = shell_optarg;
        c->in.err_msg = "cannot read script file";
        return -1;
      }

      c->have_script = 1;
      return 0;
  }

  return -1;
}

/* compiles the script; file operands and w/s///w targets (real fds) are valid
 * but not streamable (1), so a chain declines and the command runs on its own */
static int
sed_setup(void* ctx) {
  struct sed_ctx* c = ctx;
  int rc;

  if(!c->have_script) {
    if(!c->in.files) {
      c->in.err_arg = "";
      c->in.err_msg = "no script given";
      return -1;
    }

    stralloc_cats(&c->script, *c->in.files++);

    if(!*c->in.files)
      c->in.files = NULL;
  }

  if(c->autoprint_off)
    c->flags |= SED_NOAUTOPRINT;

  if((rc = sed_compile(&c->prog, c->script.s ? c->script.s : "", c->script.len, c->flags)) != SED_OK) {
    c->prog = NULL;
    c->in.err_arg = "script";
    c->in.err_msg = sed_error(rc);
    return -1;
  }

  c->nwfiles = sed_wfile_count(c->prog);

  if(!(c->st = sed_state_new(c->prog, sed_read_line, sed_out, c->nwfiles ? sed_wfile_out : NULL, sed_rfile, c))) {
    c->in.err_arg = "";
    c->in.err_msg = "out of memory";
    return -1;
  }

  return c->in.files || c->nwfiles ? 1 : 0;
}

/* w/s///w files are created before any input is processed */
static void
sed_open_wfiles(struct sed_ctx* c) {
  size_t i;

  if(!c->nwfiles || !(c->wfiles = alloc(c->nwfiles * sizeof(*c->wfiles)))) {
    c->nwfiles = 0;
    return;
  }

  for(i = 0; i < c->nwfiles; i++) {
    struct sed_wfile_rt* w = &c->wfiles[i];
    const char* name = sed_wfile_name(c->prog, i);

    byte_zero(w, sizeof(*w));
    w->fd = -1;

    if(!str_diff(name, "/dev/stdout"))
      w->alias = 1;
    else if(!str_diff(name, "/dev/stderr"))
      w->alias = 2;
    else if((w->fd = open_trunc(name)) == -1) {
      builtin_error(c->in.errargv, (char*)name);
      c->ret = 2;
    } else
      buffer_init(&w->b, &buffer_op_write, w->fd, w->wbuf, sizeof(w->wbuf));
  }
}

/* runs the script to completion on the first call (sed's control flow is not
 * resumable). With a sink the result went straight there; in a chain it is
 * handed out as one unit. */
static int
sed_step(void* arg, const char** unit, size_t* len) {
  struct sed_ctx* c = arg;

  if(!c->ran) {
    c->ran = 1;
    sed_open_wfiles(c);
    sed_run(c->st, &c->exit_status);
  }

  if(c->sent || !c->out.len)
    return 0;

  c->sent = 1;
  *unit = c->out.s;
  *len = c->out.len;
  return 1;
}

static int
sed_status(void* arg) {
  struct sed_ctx* c = arg;

  if(c->ret || c->in.had_error)
    return 2;

  return c->exit_status;
}

static void
sed_finish(void* arg) {
  struct sed_ctx* c = arg;
  size_t i;

  if(c->st)
    sed_state_free(c->st);

  for(i = 0; i < c->nwfiles && c->wfiles; i++)
    if(c->wfiles[i].alias == 0 && c->wfiles[i].fd != -1) {
      buffer_flush(&c->wfiles[i].b);
      buffer_close(&c->wfiles[i].b);
    }

  alloc_free(c->wfiles);

  if(c->prog)
    sed_free(c->prog);

  stralloc_free(&c->script);
  stralloc_free(&c->out);
}

const struct filter_ops sed_ops = {
    .opts = "nEre:f:",
    .size = sizeof(struct sed_ctx),
    .option = sed_option,
    .setup = sed_setup,
    .step = sed_step,
    .status = sed_status,
    .finish = sed_finish,
    .err_status = 2,
};

FILTER_BUILTIN(sed)