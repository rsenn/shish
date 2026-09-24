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
static int
slurp_file(const char* path, stralloc* out) {
  buffer b;
  char rbuf[4096], chunk[4096];
  ssize_t n;

  if(buffer_mmapread(&b, path) != 0) {
    int rfd = open_read(path);

    if(rfd == -1)
      return -1;

    buffer_init(&b, &buffer_op_read, rfd, rbuf, sizeof(rbuf));
  }

  while((n = buffer_get_until(&b, chunk, sizeof(chunk), "", 0)) > 0) {
    if(!stralloc_catb(out, chunk, (size_t)n)) {
      buffer_close(&b);
      return -1;
    }
  }

  buffer_close(&b);
  return (n < 0) ? -1 : 0;
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
  char** argv;   /* remaining file operands (argv+shell_optind), or NULL for stdin */
  int i;
  int done_any;
  buffer* cur;
  buffer curb;
  char rbuf[4096];
  char linebuf[8192];
  char* const* errargv; /* for builtin_error() */
  int had_error;

  struct sed_wfile_rt* wfiles;
  size_t nwfiles;
};

static int
input_open_next(struct sed_ctx* c) {
  for(;;) {
    const char* name;

    if(c->cur == &c->curb)
      buffer_close(c->cur);

    c->cur = NULL;

    if(c->argv) {
      name = c->argv[c->i];

      if(!name)
        return 0;

      c->i++;
    } else {
      if(c->done_any)
        return 0;

      name = "-";
    }

    c->done_any = 1;

    if(!str_diff(name, "-")) {
      c->cur = fd_in->r;
      return 1;
    }

    c->cur = &c->curb;

    if(buffer_mmapread(c->cur, name) == 0)
      return 1;

    {
      int rfd = open_read(name);

      if(rfd == -1) {
        builtin_error((char**)c->errargv, (char*)name);
        c->had_error = 1;
        c->cur = NULL;
        continue;
      }

      buffer_init(c->cur, &buffer_op_read, rfd, c->rbuf, sizeof(c->rbuf));
      return 1;
    }
  }
}

static int
sed_read_line(void* ctx, const char** sp, size_t* np, int* had_nl) {
  struct sed_ctx* c = ctx;

  for(;;) {
    int r;

    if(!c->cur && !input_open_next(c))
      return 0;

    r = buffer_get_until(c->cur, c->linebuf, sizeof(c->linebuf) - 1, "\n", 1);

    if(r <= 0) {
      c->cur = NULL;
      continue;
    }

    *had_nl = (c->linebuf[r - 1] == '\n');

    if(*had_nl)
      r--;

    *sp = c->linebuf;
    *np = (size_t)r;
    return 1;
  }
}

static void
sed_out(void* ctx, const char* s, size_t n) {
  (void)ctx;
  buffer_put(fd_out->w, s, n);
}

static void
sed_rfile(void* ctx, const char* name) {
  buffer b;
  char rbuf[4096], chunk[4096];
  ssize_t n;

  (void)ctx;

  if(buffer_mmapread(&b, name) != 0) {
    int rfd = open_read(name);

    if(rfd == -1)
      return; /* POSIX: a missing r file is silently ignored */

    buffer_init(&b, &buffer_op_read, rfd, rbuf, sizeof(rbuf));
  }

  while((n = buffer_get_until(&b, chunk, sizeof(chunk), "", 0)) > 0)
    buffer_put(fd_out->w, chunk, (size_t)n);

  buffer_close(&b);
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
    case 'E': case 'r': flags |= SED_ERE; break;

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
  ctx.argv = (argv[shell_optind] != NULL) ? argv + shell_optind : NULL;
  ctx.errargv = argv;
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
          buffer_init(&ctx.wfiles[i].b, &buffer_op_write, ctx.wfiles[i].fd, ctx.wfiles[i].wbuf,
                      sizeof(ctx.wfiles[i].wbuf));
        }
      }
    }
  }

  st = sed_state_new(prog, sed_read_line, sed_out, ctx.nwfiles ? sed_wfile_out : NULL, sed_rfile,
                     &ctx);

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

  if(ctx.cur == &ctx.curb)
    buffer_close(ctx.cur);

  alloc_free(ctx.wfiles);
  sed_free(prog);

  if(ctx.had_error || ret)
    return ret ? ret : 2;

  return exit_status;
}
