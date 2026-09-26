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

/* output stuff
 * ----------------------------------------------------------------------- */
const char help_xargs[] = "    Construct argument lists and invoke utility\n"
                          "\n"
                          "    -0              items are separated by \\0\n"
                          "    -a FILE         read arguments from FILE, not standard input\n"
                          "    -d CHARACTER    items in input stream are separated by CHARACTER\n"
                          "    -I REPLSTR      replace REPLSTR in INITIAL-ARGS with each input line\n"
                          "                    (split at newlines, leading blanks skipped); runs\n"
                          "                    COMMAND once per line\n"
                          "    -L MAX-LINES    use at most MAX-LINES non-blank input lines per command line\n"
                          "    -l MAX-LINES    same as -L\n"
                          "    -n MAX-ARGS     use at most MAX-ARGS arguments per command line\n"
                          "    -o              reopen stdin as /dev/tty in the child process before executing\n"
                          "                    the command; useful to run an interactive application.\n"
                          "    -p              prompt before running commands\n"
                          "    -r              if there are no arguments, then do not run COMMAND;\n"
                          "    -t              trace mode - each command is written to stderr.\n";

struct args {
  char** v;
  int c;
};

struct xargs_opts {
  unsigned no_run_noargs : 1, do_prompt : 1, reopen_pty : 1, trace:1;
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

int
builtin_xargs(int argc, char* argv[]) {
  int c, ret = 0;
  char sep = '\n', *input_file = NULL, *replstr = NULL;
  uint64 max_lines = UINT64_MAX, lines = 0;
  unsigned int max_args = UINT_MAX ;
  struct xargs_opts opts = {0, 0, 0};

  /* check options */
  while((c = shell_getopt(argc, argv, "0a:d:I:L:l:n:oP:prs:tx")) > 0) {
    switch(c) {
      case '0': sep = '\0'; break;
      case 'a': input_file = shell_optarg; break;
      case 'd': sep = shell_optarg[0]; break;
      case 'I': replstr = shell_optarg; break;
      case 'L':
      case 'l': scan_ulonglong(shell_optarg, &max_lines); break;
      case 'n': scan_uint(shell_optarg, &max_args); break;
      case 'o': opts.reopen_pty = 1; break;
      case 'p': opts.do_prompt = 1; break;
      case 'r': opts.no_run_noargs = 1; break;
      case 't': opts.trace = 1; break;
      default: builtin_invopt(argv); return 1;
    }
  }

  buffer inb, *in;
  char rbuf[1024];
  ssize_t r;

  if(!input_file) {
    in = fd_in->r;
  } else {
    in = &inb;

    if(buffer_mmapread(in, input_file)) {
      int rfd;

      if((rfd = open_read(input_file)) == -1) {
        builtin_error(argv, input_file);
        return 127;
      }

      buffer_init(in, &buffer_op_read, rfd, rbuf, sizeof(rbuf));
    }
  }

  static char* echo_argv[] = {"echo", NULL};
  struct args util = (shell_optind < argc) ? (struct args){argv + shell_optind, argc - shell_optind}
                                            : (struct args){echo_argv, 1};
  int args_cap = 64;

  if(max_args != UINT_MAX && (unsigned long)args_cap > max_args)
    args_cap = max_args + 1;

  struct args items = {NULL, 0};

  if(!(items.v = alloc(args_cap * sizeof(char*))))
    return 1;

  for(;;) {
    char buf[1024];

    if((r = buffer_get_until(in, buf, sizeof(buf) - 1, &sep, 1)) <= 0)
      break;

    buf[r] = '\0';

    if(r > 0 && buf[r - 1] == sep)
      buf[--r] = '\0';

    if(sep == '\n' && r > 0 && buf[r - 1] == '\r')
      buf[--r] = '\0';

    if(max_lines != UINT64_MAX) {
      size_t i = 0;

      while(buf[i] == ' ' || buf[i] == '\t')
        i++;

      if(!buf[i])
        continue;
    }

    if(replstr) {
      const char* line = buf;

      while(*line == ' ' || *line == '\t')
        line++;

      if(!*line)
        continue;

      int r_ret = execute_replace(util, replstr, line, opts);

      if(r_ret != 0)
        ret = r_ret;
      continue;
    }

    char* arg_copy;

    if(!(arg_copy = str_ndup(buf, r))) {
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

    int full = max_args != UINT_MAX && items.c >= (int)max_args;

    /* -L: a line ending in an unescaped blank continues onto the next one */
    if(max_lines != UINT64_MAX && !(r > 0 && (buf[r - 1] == ' ' || buf[r - 1] == '\t') && !(r > 1 && buf[r - 2] == '\\')) &&
       ++lines >= max_lines)
      full = 1;

    if(full) {
      lines = 0;
      int r_ret = execute_batch(util, items, opts);

      if(r_ret != 0)
        ret = r_ret;

      for(int i = 0; i < items.c; i++)
        alloc_free(items.v[i]);

      items.c = 0;
    }
  }

  if(!replstr && (items.c > 0 || (items.c == 0 && !opts.no_run_noargs && shell_optind >= argc))) {
    int r_ret = execute_batch(util, items, opts);

    if(r_ret != 0)
      ret = r_ret;

    for(int i = 0; i < items.c; i++)
      alloc_free(items.v[i]);
  }

  alloc_free(items.v);
  return ret;
}
