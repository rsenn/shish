#include "../builtin.h"
#include "../fdtable.h"
#include "../exec.h"
#include "../var.h"
#include "../../lib/shell.h"
#include "../../lib/scan.h"
#include "../../lib/str.h"
#include "../../lib/alloc.h"
#include "../../lib/byte.h"
#include "../../lib/unix.h"
#include "../../lib/wait.h"
#include "../../lib/sig.h"
#include <errno.h>
#include <signal.h>
#include <unistd.h>

const char help_timeout[] =
    "    Run COMMAND, killing it if it's still running after DURATION.\n"
    "\n"
    "    -k, --kill-after=DURATION   also send KILL this long after the\n"
    "                                first signal, if COMMAND is still running\n"
    "    -s, --signal=SIGNAL         signal to send on timeout (default TERM)\n"
    "    -v, --verbose               report to stderr what signal was sent\n"
    "    DURATION                    seconds to allow, fractional, with an\n"
    "                                optional s/m/h/d suffix; 0 disables it\n"
    "    COMMAND [ARG]...            program to run\n";

/* removes 'n' argv slots starting at 'i', shifting the rest down;
 * 'argc' is adjusted in place.
 * ----------------------------------------------------------------------- */
static void
remove_args(char* argv[], int* argc, int i, int n) {
  byte_copyr(&argv[i], (*argc - i - n) * sizeof(char*), &argv[i + n]);
  *argc -= n;
}

/* pulls "NAME VALUE" or "NAME=VALUE" out of argv, wherever it
 * appears, returning VALUE, NULL if absent, or (char*)-1 if NAME was
 * given without a value.
 * ----------------------------------------------------------------------- */
static char*
extract_longopt(char* argv[], int* argc, const char* name) {
  size_t namelen = str_len(name);
  int i;

  for(i = 1; i < *argc; i++) {
    if(str_equal(argv[i], name)) {
      char* val = argv[i + 1];

      if(!val)
        return (char*)-1;

      remove_args(argv, argc, i, 2);
      return val;
    }

    if(!str_diffn(argv[i], name, namelen) && argv[i][namelen] == '=') {
      char* val = argv[i] + namelen + 1;

      remove_args(argv, argc, i, 1);
      return val;
    }
  }

  return NULL;
}

/* parses "NUMBER[.FRACTION][smhd]" (the same grammar as the sleep
 * builtin) into whole microseconds. Returns 0 on success, -1 if 's'
 * isn't a valid duration.
 * ----------------------------------------------------------------------- */
static int
timeout_parse_duration(const char* s, unsigned long* usec_total) {
  unsigned long intpart = 0, frac = 0;
  size_t n, fraclen = 0, i;
  double seconds, mult, scale = 1.0;

  n = scan_ulong(s, &intpart);
  s += n;

  if(*s == '.') {
    s++;
    fraclen = scan_ulong(s, &frac);
    s += fraclen;
  }

  if(n == 0 && fraclen == 0)
    return -1;

  switch(*s) {
    case '\0': mult = 1.0; break;
    case 's':
      mult = 1.0;
      s++;
      break;
    case 'm':
      mult = 60.0;
      s++;
      break;
    case 'h':
      mult = 3600.0;
      s++;
      break;
    case 'd':
      mult = 86400.0;
      s++;
      break;
    default: return -1;
  }

  if(*s != '\0')
    return -1;

  for(i = 0; i < fraclen; i++)
    scale *= 10.0;

  seconds = ((double)intpart + (double)frac / scale) * mult;
  *usec_total = (unsigned long)(seconds * 1000000.0 + 0.5);
  return 0;
}

/* parses a "-s"/"--signal" operand: numeric ("9") or a name ("TERM"/
 * "SIGTERM", case-insensitive, with or without "SIG"). Returns the
 * signal number, or -1 if spec is neither.
 * ----------------------------------------------------------------------- */
static int
timeout_signum(const char* spec) {
  int n = 0;

  if(spec[0] && scan_int(spec, &n) == str_len(spec))
    return n;

  return sig_byname(spec);
}

int
builtin_timeout(int argc, char* argv[]) {
  int c, verbose = 0, ret;
  char *kill_after_arg = NULL, *signal_arg = NULL, *path, **cmdargv;
  unsigned long duration_usec = 0, kill_after_usec = 0;
  int pid, sig = SIGTERM;

  kill_after_arg = extract_longopt(argv, &argc, "--kill-after");
  signal_arg = extract_longopt(argv, &argc, "--signal");

  if(kill_after_arg == (char*)-1 || signal_arg == (char*)-1) {
    builtin_errmsg(argv, "option requires an argument", NULL);
    return 125;
  }

  while((c = shell_getopt(argc, argv, "k:s:v")) > 0) {
    switch(c) {
      case 'k': kill_after_arg = shell_optarg; break;
      case 's': signal_arg = shell_optarg; break;
      case 'v': verbose = 1; break;
      default: builtin_invopt(argv); return 125;
    }
  }

  if(argc - shell_optind < 2) {
    builtin_errmsg(argv, "missing operand", NULL);
    return 125;
  }

  if(timeout_parse_duration(argv[shell_optind], &duration_usec) == -1) {
    builtin_errmsg(argv, argv[shell_optind], "invalid time interval");
    return 125;
  }

  if(kill_after_arg && timeout_parse_duration(kill_after_arg, &kill_after_usec) == -1) {
    builtin_errmsg(argv, kill_after_arg, "invalid time interval");
    return 125;
  }

  if(signal_arg) {
    sig = timeout_signum(signal_arg);

    if(sig <= 0) {
      builtin_errmsg(argv, signal_arg, "invalid signal");
      return 125;
    }
  }

  cmdargv = &argv[shell_optind + 1];
  path = cmdargv[0][str_chr(cmdargv[0], '/')] ? cmdargv[0] : exec_path(cmdargv[0]);

  if(!path) {
    builtin_errmsg(argv, cmdargv[0], "command not found");
    return 127;
  }

  if(access(path, X_OK) == -1) {
    int notfound = errno == ENOENT;

    builtin_error(argv, cmdargv[0]);
    return notfound ? 127 : 126;
  }

  /* block SIGCHLD across the fork and the whole wait loop below --
   * otherwise the shell's own SIGCHLD handler (sh_onsig(), installed
   * for job control) can reap this child first, since it isn't
   * registered in the job table, silently discarding its exit
   * status before wait_pid_nohang() ever gets a chance to see it. */
  sig_block(SIGCHLD);

  pid = fork();

  if(pid == -1) {
    sig_unblock(SIGCHLD);
    builtin_error(argv, cmdargv[0]);
    return 125;
  }

  if(pid == 0) {
    unsigned long envn = var_count(V_EXPORT) + 1;
    char** envp = var_export(alloc(envn * sizeof(char*)));

    sig_unblock(SIGCHLD);
    execve(path, cmdargv, envp);
    _exit(126);
  }

  {
    int wstat = 0, r, sig_sent = 0;
    unsigned long elapsed = 0;

    for(;;) {
      r = wait_pid_nohang(pid, &wstat);

      if(r == pid)
        break;

      if(r == -1) {
        ret = 125;
        goto done;
      }

      usleep(1000);
      elapsed += 1000;

      if(!sig_sent && duration_usec && elapsed >= duration_usec) {
        if(verbose) {
          buffer_puts(fd_err->w, "timeout: sending signal ");
          buffer_puts(fd_err->w, sig_name(sig));
          buffer_puts(fd_err->w, " to command '");
          buffer_puts(fd_err->w, cmdargv[0]);
          buffer_puts(fd_err->w, "'");
          buffer_putnlflush(fd_err->w);
        }

        kill(pid, sig);
        sig_sent = 1;
        elapsed = 0;
      } else if(sig_sent == 1 && kill_after_usec && elapsed >= kill_after_usec) {
        if(verbose) {
          buffer_puts(fd_err->w, "timeout: sending signal KILL to command '");
          buffer_puts(fd_err->w, cmdargv[0]);
          buffer_puts(fd_err->w, "'");
          buffer_putnlflush(fd_err->w);
        }

        kill(pid, SIGKILL);
        sig_sent = 2;
      }
    }

    if(sig_sent && WAIT_IF_SIGNALED(wstat) && WAIT_TERMSIG(wstat) == SIGKILL)
      ret = 137;
    else if(sig_sent)
      ret = 124;
    else
      ret = WAIT_STATUS(wstat);
  }

done:
  sig_unblock(SIGCHLD);
  return ret;
}
