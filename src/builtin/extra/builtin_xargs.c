#include "../../builtin.h"
#include "../../fdtable.h"
#include "../../../lib/shell.h"
#include "../../../lib/scan.h"
#include "../../../lib/open.h"
#include "../../../lib/alloc.h"
#include "../../../lib/str.h"
#include <sys/wait.h>

/* output stuff
 * ----------------------------------------------------------------------- */
const char help_xargs[] = "    Locate a command on $PATH.\n"
                          "\n"
                          "    -0              items are separated by \\0\n"
                          "    -a FILE         read arguments from FILE, not standard input\n"
                          "    -d CHARACTER    items in input stream are separated by CHARACTER\n"
                                 /*"    -I R            same as -i R\n"
                          "    -i R            replace R in INITIAL-ARGS with names read from standard input,\n"
                          "                    split at newlines; if R is unspecified, assume {}\n"*/
                          "    -L MAX-LINES    use at most MAX-LINES non-blank input lines per command line\n"
                          "    -l[MAX-LINES]   similar to -L but defaults to at most one non-\n"
                          "                    blank input line if MAX-LINES is not specified\n"
                          "    -n MAX-ARGS     use at most MAX-ARGS arguments per command line\n"
                          "    -o              reopen stdin as /dev/tty in the child process before executing\n"
                          "                    the command; useful to run an interactive application.\n"
                          "    -P MAX-PROCS    run at most MAX-PROCS processes at a time\n"
                          "    -p              prompt before running commands\n"
                          "    -r              if there are no arguments, then do not run COMMAND;\n"
                          "    -t              trace mode - each command is written to stderr.\n";

struct args {
  char** v;
  int c;
};

struct flags {
  unsigned no_run_noargs : 1, do_prompt : 1, reopen_pty : 1, trace:1;
};

static int
execute_batch(struct args cmd, int optind, int init_argc, struct args x, struct flags opt) {
  if(x.c == 0 && opt.no_run_noargs)
    return 0;

  int argc = init_argc + x.c;
  char** argv;
  pid_t pid;

  if(!(argv = alloc((argc + 1) * sizeof(char*))))
    return 1;

  if(init_argc > 0 && optind < cmd.c) {
    for(int i = 0; i < init_argc; i++)
      argv[i] = cmd.v[optind + i];
  } else {
    argv[0] = "echo";
  }

  for(int i = 0; i < x.c; i++)
    argv[init_argc + i] = x.v[i];
  argv[argc] = NULL;

  if(opt.do_prompt) {
    buffer_puts(fd_err->w, argv[0]);

    for(int i = 1; i < argc; i++) {
      buffer_puts(fd_err->w, " ");
      buffer_puts(fd_err->w, argv[i]);
    }

    buffer_putsflush(fd_err->w, "?. ");

    char c_resp = 0;
    buffer_getc(fd_in->r, &c_resp);
    if(c_resp != 'y' && c_resp != 'Y') {
      alloc_free(argv);
      return 0;
    }
  }

  if((pid = fork()) < 0) {
    alloc_free(argv);
    return 1;
  }

  if(pid == 0) {
    if(opt.reopen_pty) {
      int tty_fd;

      if((tty_fd = open("/dev/tty", 2)) != -1) {
        dup2(tty_fd, 0);

        if(tty_fd > 2)
          close(tty_fd);
      }
    }

    execvp(argv[0], argv);
    exit(127);
  } else {
    int status;
    waitpid(pid, &status, 0);
    alloc_free(argv);

    if(WIFEXITED(status)) {
      int exit_code;

      if((exit_code = WEXITSTATUS(status)) != 0) {
        if(exit_code == 127)
          return 127;

        return exit_code;
      }
    }
  }

  return 0;
}

int
builtin_xargs(int argc, char* argv[]) {
  int c, ret = 0;
  char sep = '\n', *input_file = NULL;
  uint64 max_lines = UINT64_MAX;
  unsigned int max_args = UINT_MAX, max_procs = UINT_MAX;
  struct flags opt = {0, 0, 0};
  struct args cmd = {argv, argc};

  /* check options */
  while((c = shell_getopt(cmd.c, cmd.v, "0a:d:" /*"E:eI:i:"*/ "L:l:n:oP:prs:tx")) > 0) {
    switch(c) {
      case '0': sep = '\0'; break;
      case 'a': input_file = shell_optarg; break;
      case 'd': sep = shell_optarg[0]; break;
      case 'L':
      case 'l': scan_ulonglong(shell_optarg, &max_lines); break;
      case 'n': scan_uint(shell_optarg, &max_args); break;
      case 'o': opt.reopen_pty = 1; break;
      case 'P': scan_uint(shell_optarg, &max_procs); break;
      case 'p': opt.do_prompt = 1; break;
      case 'r': opt.no_run_noargs = 1; break;
      case 't': opt.trace = 1; break;
      default: builtin_invopt(cmd.v); return 1;
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
        builtin_error(cmd.v, input_file);
        return 127;
      }

      buffer_init(in, &buffer_op_read, rfd, rbuf, sizeof(rbuf));
    }
  }

  int init_argc = (shell_optind < cmd.c) ? (cmd.c - shell_optind) : 1;
  int args_cap = 64;

  if(max_args != UINT_MAX && (unsigned long)args_cap > max_args)
    args_cap = max_args + 1;

  struct args x;

  if(!(x.v = alloc(args_cap * sizeof(char*))))
    return 1;

  for(;;) {
    char buf[1024];

    if((r = buffer_get_until(in, buf, sizeof(buf) - 1, &sep, 1)) <= 0)
      break;

    buf[r] = '\0';

    if(sep == '\n') {
      if(r > 0 && buf[r - 1] == '\n')
        buf[--r] = '\0';
      if(r > 0 && buf[r - 1] == '\r')
        buf[--r] = '\0';
    }

    char* arg_copy;

    if(!(arg_copy = str_ndup(buf, r))) {
      ret = 1;
      break;
    }

    if(x.c >= args_cap) {
      char** new_args;
      args_cap *= 2;

      if(!(new_args = alloc_re(x.v, args_cap * sizeof(char*)))) {
        alloc_free(arg_copy);
        ret = 1;
        break;
      }

      x.v = new_args;
    }

    x.v[x.c++] = arg_copy;

    if(max_args != UINT_MAX && x.c >= (int)max_args) {
      int r_ret = execute_batch(cmd, shell_optind, init_argc, x, opt);

      if(r_ret != 0)
        ret = r_ret;

      for(int i = 0; i < x.c; i++)
        alloc_free(x.v[i]);

      x.c = 0;
    }
  }

  if(x.c > 0 || (x.c == 0 && !opt.no_run_noargs && shell_optind >= cmd.c)) {
    int r_ret = execute_batch(cmd, shell_optind, init_argc, x, opt);

    if(r_ret != 0)
      ret = r_ret;

    for(int i = 0; i < x.c; i++)
      alloc_free(x.v[i]);
  }

  alloc_free(x.v);
  return ret;
}
