#include "../../builtin.h"
#include "../../fdtable.h"
#include "../../../text/awk.h"
#include "../../../lib/shell.h"
#include "../../../lib/str.h"
#include "../../../lib/byte.h"
#include "../../../lib/alloc.h"
#include "../../../lib/open.h"
#include "../../../lib/stralloc.h"
#include <stdint.h> /* intptr_t */
#include <unistd.h> /* read/write/close for files opened directly by this builtin */

const char help_awk[] = "    Pattern scanning and processing language (a subset -- see doc/).\n"
                        "\n"
                        "    -F sep          set FS before BEGIN runs\n"
                        "    -v name=value   assign a variable before BEGIN runs\n"
                        "    -f progfile     read the program from progfile (repeatable)\n"
                        "    program         the program text, if -f was not given\n"
                        "    file            input file(s); '-' or omitted means stdin\n"
                        "\n"
                        "    Not implemented: cmd | getline, print | cmd, system() (all three\n"
                        "    would need the shell's own process model wired in); nextfile is\n"
                        "    supported but paragraph mode (RS=\"\") is not.\n";

/* slurp_file: appends the whole of path to *out. 0 on success, -1 on
 * error (path could not be opened or read). Shared shape with
 * builtin_sed.c's own copy -- small enough not to be worth a common
 * home. */
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

/* three fixed sentinel handles standing in for the shell's own stdin/
 * stdout/stderr (fd_in/fd_out/fd_err), so open_read()/open_write()
 * never allocate a handle indistinguishable from one of these -- an
 * awk program's redirection to "/dev/stdout"/"/dev/stderr", its main
 * input's "-"/no-operand case, and getline<"-" all alias the shell's
 * own fds instead of reopening them. Every other handle is a real fd,
 * encoded as (void*)(intptr_t)(fd + 1) so fd 0 is never NULL. */
static int awk_stdin_tag, awk_stdout_tag, awk_stderr_tag;
#define STDIN_H (&awk_stdin_tag)
#define STDOUT_H (&awk_stdout_tag)
#define STDERR_H (&awk_stderr_tag)

static void*
awk_open_read(void* ctx, const char* name) {
  int fd;

  (void)ctx;

  if(!str_diff(name, "-"))
    return STDIN_H;

  fd = open_read(name);
  return fd < 0 ? NULL : (void*)(intptr_t)(fd + 1);
}

static long
awk_read(void* ctx, void* h, char* buf, size_t len) {
  (void)ctx;

  if(h == STDIN_H) {
    /* the shell's own stdin is a buffer (shared position with the
       `read` builtin and any prior buffering), not a bare fd, so this
       must go through fd_in->r rather than read(2) on its raw fd. */
    size_t i;

    for(i = 0; i < len; i++) {
      char c;
      int r = buffer_getc(fd_in->r, &c);

      if(r <= 0)
        return (long)i;

      buf[i] = c;
    }

    return (long)len;
  }

  return (long)read((int)((intptr_t)h - 1), buf, len);
}

static void
awk_close_read(void* ctx, void* h) {
  (void)ctx;

  if(h == STDIN_H)
    return; /* never close the shell's own stdin */

  close((int)((intptr_t)h - 1));
}

static void*
awk_open_write(void* ctx, const char* name, int append) {
  int fd;

  (void)ctx;

  if(!str_diff(name, "/dev/stdout"))
    return STDOUT_H;

  if(!str_diff(name, "/dev/stderr"))
    return STDERR_H;

  fd = append ? open_append(name) : open_trunc(name);
  return fd < 0 ? NULL : (void*)(intptr_t)(fd + 1);
}

static int
awk_write(void* ctx, void* h, const char* s, size_t n) {
  (void)ctx;

  if(h == STDOUT_H) {
    if(n)
      buffer_put(fd_out->w, s, n);
    else
      buffer_flush(fd_out->w);

    return (int)n;
  }

  if(h == STDERR_H) {
    if(n)
      buffer_put(fd_err->w, s, n);
    else
      buffer_flush(fd_err->w);

    return (int)n;
  }

  if(!h)
    return 0;

  if(n == 0)
    return 0; /* a real fd's writes are never buffered here */

  return (int)write((int)((intptr_t)h - 1), s, n);
}

static void
awk_close_write(void* ctx, void* h) {
  (void)ctx;

  if(h == STDOUT_H || h == STDERR_H)
    return; /* never close the shell's own stdout/stderr */

  close((int)((intptr_t)h - 1));
}

int
builtin_awk(int argc, char* argv[]) {
  int c;
  const char* fs = NULL;
  char* assigns[64];
  int nassigns = 0;
  int have_prog = 0;
  stralloc prog;
  struct awk_prog* compiled;
  unsigned long errline = 0;
  int rc, status;
  struct awk_io io;

  stralloc_init(&prog);

  while((c = shell_getopt(argc, argv, "F:v:f:")) > 0) {
    switch(c) {
      case 'F': fs = shell_optarg; break;

      case 'v':
        if(nassigns < (int)(sizeof(assigns) / sizeof(assigns[0])) - 1)
          assigns[nassigns++] = shell_optarg;
        break;

      case 'f':
        if(have_prog)
          stralloc_catc(&prog, '\n');

        if(slurp_file(shell_optarg, &prog) == -1) {
          builtin_error(argv, shell_optarg);
          stralloc_free(&prog);
          return 2;
        }

        have_prog = 1;
        break;

      default: builtin_invopt(argv); return 2;
    }
  }

  assigns[nassigns] = NULL;

  if(!have_prog) {
    if(!argv[shell_optind]) {
      builtin_error(argv, "no program given");
      stralloc_free(&prog);
      return 2;
    }

    stralloc_cats(&prog, argv[shell_optind++]);
  }

  rc = awk_compile(&compiled, prog.s ? prog.s : "", prog.len, &errline);
  stralloc_free(&prog);

  if(rc != AWK_OK) {
    static const char msg[] = "syntax error at source line";

    builtin_errmsgn_nonl(argv, msg, sizeof(msg) - 1, NULL);
    buffer_putspace(fd_err->w);
    buffer_putulong(fd_err->w, errline);
    buffer_putnlflush(fd_err->w);
    return 2;
  }

  byte_zero(&io, sizeof(io));
  io.open_read = &awk_open_read;
  io.read = &awk_read;
  io.close_read = &awk_close_read;
  io.open_write = &awk_open_write;
  io.write = &awk_write;
  io.close_write = &awk_close_write;
  io.run_shell = NULL; /* cmd|getline, print|cmd, system(): not supported (see help_awk) */
  io.out = STDOUT_H;
  io.err = STDERR_H;
  io.ctx = NULL;

  status = awk_run(compiled, &io, assigns, argv + shell_optind, fs);
  awk_free(compiled);
  return status;
}
