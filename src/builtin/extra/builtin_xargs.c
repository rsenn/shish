#include "builtin_config.h"

#if BUILTIN_XARGS

#include "../../builtin.h"
#include "../../exec.h"
#include "../../fd.h"
#include "../../fdstack.h"
#include "../../fdtable.h"
#include "../../sh.h"
#include "../../../lib/shell.h"
#include "../../../lib/scan.h"
#include "../../../lib/open.h"
#include "../../../lib/alloc.h"
#include "../../../lib/str.h"
#include "../../../lib/stralloc.h"

/* output stuff
 * ----------------------------------------------------------------------- */
const char help_xargs[] = "    Construct argument lists and invoke utility\n"
                          "\n"
                          "    -0              items are separated by \\0\n"
                          "    -a FILE         read arguments from FILE, not standard input\n"
                          "    -d CHARACTER    items in input stream are separated by CHARACTER\n"
                          "    -E EOFSTR       stop reading at an argument equal to EOFSTR ('' disables)\n"
                          "    -I REPLSTR      replace REPLSTR in INITIAL-ARGS with each input line\n"
                          "                    (split at newlines, leading blanks skipped); runs\n"
                          "                    COMMAND once per line\n"
                          "    -L MAX-LINES    use at most MAX-LINES non-blank input lines per command line\n"
                          "    -l MAX-LINES    same as -L\n"
                          "    -n MAX-ARGS     use at most MAX-ARGS arguments per command line\n"
                          "    -o              reopen stdin as /dev/tty in the child process before executing\n"
                          "                    the command; useful to run an interactive application.\n"
                          "    -p              prompt before running commands\n"
                          "    -s SIZE         command lines stay below SIZE bytes (default 131072); an\n"
                          "                    argument that cannot fit on its own is an error\n"
                          "    -x              exit if -n/-L arguments do not fit in SIZE\n"
                          "    -r              if there are no arguments, then do not run COMMAND;\n"
                          "    -t              trace mode - each command is written to stderr.\n"
                          "\n"
                          "    Input arguments are separated by blanks and newlines; '...', \"...\" and\n"
                          "    backslash quote (not with -0/-d). With no input the utility runs once\n"
                          "    (not with -r or -I).\n";

struct args {
  char** v;
  int c;
};

#define XARGS_SIZE 131072 /* default -s: bytes of utility + arguments, each counted with its NUL */

struct xargs_opts {
  unsigned no_run_noargs : 1;
  unsigned do_prompt : 1;
  unsigned reopen_pty : 1;
  unsigned trace : 1;
};

static int
execute_batch(struct args util, struct args items, struct xargs_opts opts) {
  if(items.c == 0 && opts.no_run_noargs)
    return 0;

  int argc = util.c + items.c;
  char** argv;
  pid_t pid;

  if(!(argv = alloc((argc + 1) * sizeof(char*))))
    return 1;

  for(int i = 0; i < util.c; i++)
    argv[i] = util.v[i];

  for(int i = 0; i < items.c; i++)
    argv[util.c + i] = items.v[i];
  argv[argc] = NULL;

  if(opts.trace || opts.do_prompt) {
    buffer_puts(fd_err->w, argv[0]);

    for(int i = 1; i < argc; i++) {
      buffer_puts(fd_err->w, " ");
      buffer_puts(fd_err->w, argv[i]);
    }

    buffer_putsflush(fd_err->w, opts.do_prompt ? "?. " : "\n");
  }

  if(opts.do_prompt) {
    char c_resp = 0;
    buffer_getc(fd_in->r, &c_resp);
    if(c_resp != 'y' && c_resp != 'Y') {
      alloc_free(argv);
      return 0;
    }
  }

  /* run through the shell's own dispatch, so builtins, functions and
     programs all work and "$(...)" capture is handled by exec_program().
     stdin is /dev/null (or the tty with -o), not the item stream. */
  struct fdstack io;
  struct fd* in;
  char buf[FD_BUFSIZE];
  struct command cmd = exec_hash(argv[0], 0);
  int ret;

  fdstack_push(&io);
  in = fd_push(fd_alloc(), STDIN_FILENO, FD_READ);
  fd_open(in, opts.reopen_pty ? "/dev/tty" : "/dev/null", 0);

  if(fd_needbuf(in))
    fd_setbuf(in, buf, sizeof(buf));

  if(cmd.ptr) {
    ret = exec_command(&cmd, argc, argv, 0);
  } else {
    buffer_putm_internal(fd_err->w, "xargs: ", argv[0], ": ", strerror(exec_lasterrno), 0);
    buffer_putnlflush(fd_err->w);
    ret = EXIT_NOTFOUND;
  }

  fdstack_pop(&io);
  alloc_free(argv);
  return ret;
}

/* runs the command once with every occurrence of replstr in the initial
 * arguments replaced by line (insert mode, -I). */
static int
execute_replace(struct args util, const char* replstr, const char* line, struct xargs_opts opts) {
  size_t rl = str_len(replstr), ll = str_len(line);
  char** nv;
  int ret = 1;

  if(!(nv = alloc((util.c + 1) * sizeof(char*))))
    return 1;

  int n = 0;

  for(; n < util.c; n++) {
    const char* a = util.v[n];
    size_t i, len = 0;
    char *o, *w;

    for(i = 0; a[i];)
      if(rl && !str_diffn(a + i, replstr, rl)) {
        len += ll;
        i += rl;
      } else {
        len++;
        i++;
      }

    if(!(o = alloc(len + 1)))
      goto done;

    for(w = o, i = 0; a[i];)
      if(rl && !str_diffn(a + i, replstr, rl)) {
        for(size_t k = 0; k < ll; k++)
          *w++ = line[k];
        i += rl;
      } else {
        *w++ = a[i++];
      }

    *w = '\0';
    nv[n] = o;
  }

  struct args sub = {nv, util.c}, no_items = {NULL, 0};

  opts.no_run_noargs = 0;
  ret = execute_batch(sub, no_items, opts);

done:
  while(n-- > 0)
    alloc_free(nv[n]);
  alloc_free(nv);
  return ret;
}

/* the input, with one character of pushback */
struct xargs_in {
  buffer* b;
  int pushed; /* -1 = none */
};

static int
xargs_getc(struct xargs_in* in) {
  char c;

  if(in->pushed >= 0) {
    c = in->pushed;
    in->pushed = -1;
    return (unsigned char)c;
  }

  return buffer_getc(in->b, &c) > 0 ? (unsigned char)c : -1;
}

/* the next character; "\r\n" is read as "\n" */
static int
xargs_getc_nl(struct xargs_in* in) {
  int c = xargs_getc(in), d;

  if(c == '\r') {
    if((d = xargs_getc(in)) == '\n')
      return '\n';

    in->pushed = d;
  }

  return c;
}

/* reads the next argument into out.
 *
 *   mode 0   blanks and newlines separate; quotes and backslash are processed
 *   mode 1   only newlines separate (-I): blanks inside stay, leading ones go
 *   mode 2   only sep separates, nothing is quoted (-0, -d)
 *
 * returns 1 for an argument, 0 at end of input, -1 for an unmatched quote.
 * *eol is 1 when the argument ends its line, 0 when a trailing blank continues the line (-L).
 * ----------------------------------------------------------------------- */
static int
xargs_token(struct xargs_in* in, stralloc* out, int mode, int sep, int* eol) {
  int c, have = 0;

  stralloc_zero(out);
  *eol = 1;

  for(;;) {
    c = mode == 2 ? xargs_getc(in) : xargs_getc_nl(in);

    if(c < 0)
      return have ? 1 : 0;

    if(mode == 2) {
      if(c == sep)
        return 1;

      stralloc_catb(out, (char*)&c, 1);
      have = 1;
      continue;
    }

    if(c == '\n') {
      if(have)
        return 1;

      continue;
    }

    if(c == ' ' || c == '\t') {
      if(mode == 1 && have) {
        stralloc_catb(out, (char*)&c, 1);
        continue;
      }

      if(!have)
        continue;

      /* a blank ends the argument; blanks right before the newline continue the line */
      while((c = xargs_getc(in)) == ' ' || c == '\t')
        ;

      if(c == '\r') {
        int d = xargs_getc(in);

        if(d == '\n')
          c = '\n';
        else
          in->pushed = d;
      }

      if(c == '\n')
        *eol = 0;
      else
        in->pushed = c;

      return 1;
    }

    if(c == '\\') {
      if((c = xargs_getc(in)) < 0)
        c = '\\';

      stralloc_catb(out, (char*)&c, 1);
      have = 1;
      continue;
    }

    if(c == '\'' || c == '"') {
      int q = c;

      have = 1;

      while((c = xargs_getc(in)) != q) {
        if(c < 0 || c == '\n') {
          buffer_putm_internal(fd_err->w, "xargs: unmatched ", q == '"' ? "double" : "single", " quote", 0);
          buffer_putnlflush(fd_err->w);
          return -1;
        }

        stralloc_catb(out, (char*)&c, 1);
      }

      continue;
    }

    stralloc_catb(out, (char*)&c, 1);
    have = 1;
  }
}

int
builtin_xargs(int argc, char* argv[]) {
  int c, ret = 0, ran = 0, eol;
  int sepmode = 0;
  char sep = '\n', *input_file = NULL, *replstr = NULL, *eofstr = NULL;
  uint64 max_lines = UINT64_MAX, lines = 0;
  unsigned int max_args = UINT_MAX;
  unsigned long max_size = XARGS_SIZE;
  struct xargs_opts opts = {0, 0, 0, 0};
  unsigned exit_if_size = 0;

  /* check options */
  while((c = shell_getopt(argc, argv, "0a:d:E:I:L:l:n:oP:prs:tx")) > 0) {
    switch(c) {
      case '0': sep = '\0', sepmode = 2; break;
      case 'a': input_file = shell_optarg; break;
      case 'd': sep = shell_optarg[0], sepmode = 2; break;
      case 'E': eofstr = shell_optarg; break;
      case 'I': replstr = shell_optarg; break;
      case 'L':
      case 'l': scan_ulonglong(shell_optarg, &max_lines); break;
      case 'n': scan_uint(shell_optarg, &max_args); break;
      case 'o': opts.reopen_pty = 1; break;
      case 'p': opts.do_prompt = 1; break;
      case 'r': opts.no_run_noargs = 1; break;
      case 's': scan_ulong(shell_optarg, &max_size); break;
      case 't': opts.trace = 1; break;
      case 'x': exit_if_size = 1; break;
      default: builtin_invopt(argv); return 1;
    }
  }

  if(replstr) {
    exit_if_size = 1;

    if(!sepmode)
      sepmode = 1;
  }

  buffer inb, *inbuf;
  char rbuf[1024];

  if(!input_file) {
    inbuf = fd_in->r;
  } else {
    inbuf = &inb;

    if(buffer_mmapread(inbuf, input_file)) {
      int rfd;

      if((rfd = open_read(input_file)) == -1) {
        builtin_error(argv, input_file);
        return 127;
      }

      buffer_init(inbuf, &buffer_op_read, rfd, rbuf, sizeof(rbuf));
    }
  }

  struct xargs_in in = {inbuf, -1};
  stralloc tok;
  static char* echo_argv[] = {"echo", NULL};
  struct args util =
      (shell_optind < argc) ? (struct args){argv + shell_optind, argc - shell_optind} : (struct args){echo_argv, 1};
  unsigned long base = 0, used;
  int args_cap = 64, t;

  for(c = 0; c < util.c; c++)
    base += str_len(util.v[c]) + 1;

  used = base;

  if(max_args != UINT_MAX && (unsigned long)args_cap > max_args)
    args_cap = max_args + 1;

  struct args items = {NULL, 0};

  if(!(items.v = alloc(args_cap * sizeof(char*))))
    return 1;

  stralloc_init(&tok);

  while((t = xargs_token(&in, &tok, sepmode, sep, &eol)) > 0) {
    char* arg_copy;
    int full;

    stralloc_nul(&tok);

    if(eofstr && sepmode != 2 && eofstr[0] && !str_diff(tok.s, eofstr))
      break;

    if(replstr) {
      int r_ret = execute_replace(util, replstr, tok.s, opts);

      ran++;

      if(r_ret != 0)
        ret = r_ret;
      continue;
    }

    /* the argument does not fit behind the ones already collected */
    if(items.c && used + tok.len + 1 >= max_size) {
      if(exit_if_size && ((max_args != UINT_MAX && items.c < (int)max_args) || (max_lines != UINT64_MAX && lines < max_lines))) {
        buffer_putsflush(fd_err->w, "xargs: argument line too long\n");
        ret = 1;
        goto done;
      }

      ran++;
      if((c = execute_batch(util, items, opts)) != 0)
        ret = c;

      for(c = 0; c < items.c; c++)
        alloc_free(items.v[c]);

      items.c = 0;
      lines = 0;
      used = base;
    }

    if(base + tok.len + 1 >= max_size) {
      buffer_putsflush(fd_err->w, "xargs: argument too long\n");
      ret = 1;
      goto done;
    }

    if(!(arg_copy = str_ndup(tok.s, tok.len))) {
      ret = 1;
      break;
    }

    if(items.c >= args_cap) {
      char** new_args;
      args_cap *= 2;

      if(!(new_args = alloc_re(items.v, args_cap * sizeof(char*)))) {
        alloc_free(arg_copy);
        ret = 1;
        break;
      }

      items.v = new_args;
    }

    items.v[items.c++] = arg_copy;
    used += tok.len + 1;

    full = max_args != UINT_MAX && items.c >= (int)max_args;

    /* -L: a line ends at its newline unless a trailing blank continues it */
    if(max_lines != UINT64_MAX && eol && ++lines >= max_lines)
      full = 1;

    if(full) {
      lines = 0;
      ran++;

      if((c = execute_batch(util, items, opts)) != 0)
        ret = c;

      for(c = 0; c < items.c; c++)
        alloc_free(items.v[c]);

      items.c = 0;
      used = base;
    }
  }

  if(t < 0) {
    ret = 1;
    goto done;
  }

  /* the rest, or the one run that empty input still gets */
  if(!replstr && (items.c > 0 || (!ran && !opts.no_run_noargs))) {
    if((c = execute_batch(util, items, opts)) != 0)
      ret = c;
  }

done:
  for(c = 0; c < items.c; c++)
    alloc_free(items.v[c]);

  alloc_free(items.v);
  stralloc_free(&tok);
  return ret;
}
#endif /* BUILTIN_XARGS */
