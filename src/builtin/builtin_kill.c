#include "../builtin.h"
#include "../job.h"
#include "../fdtable.h"
#include "../../lib/scan.h"
#include "../../lib/str.h"
#include "../../lib/sig.h"
#include "../../lib/sig_internal.h"
#include "../../lib/buffer.h"
#include "../../lib/unix.h"
#include <signal.h>

/* parse a "-signal" operand: numeric ("-9") or a name ("-TERM"/
 * "-SIGTERM", case-insensitive, with or without "SIG"). Returns the
 * signal number, or -1 if spec is neither.
 * ----------------------------------------------------------------------- */
static int
kill_signum(const char* spec) {
  int n = 0;

  if(spec[0] && scan_int(spec, &n) == str_len(spec))
    return n;

  return sig_byname(spec);
}

/* send 'sig' to every process of a job resolved from a "%..." operand
 * ----------------------------------------------------------------------- */
static int
kill_job(char* argv[], const char* spec, int sig) {
  struct job* j;
  size_t p;

  if(!(j = job_find(spec)))
    return builtin_errmsg(argv, (char*)spec, "no such job");

  if(j->pgrp)
    return killpg(j->pgrp, sig) == -1 ? builtin_error(argv, (char*)spec) : 0;

  for(p = 0; p < j->nproc; p++)
    if(j->procs[p].pid > 0)
      kill(j->procs[p].pid, sig);

  return 0;
}

/* kill builtin: send a signal to one or more processes or jobs
 *
 * usage: kill [-signal|-number] pid|%job ...
 * ----------------------------------------------------------------------- */
const char help_kill[] = "    Send a signal to processes or jobs.\n"
                         "\n"
                         "    -signal         signal name or number to send (default TERM)\n"
                         "    -s signal       signal name or number to send\n"
                         "    -l              list signal names\n"
                         "    pid             process ID to signal\n"
                         "    %job            job to signal (every process in its group)\n";

/* list signal names to stdout
 * ----------------------------------------------------------------------- */
static int
kill_list(void) {
  const sigtable_t* p;

  /* skip sigtable[0], the "EXIT" pseudo-signal -- kill -l lists real
     signals only */
  for(p = sigtable + 1; p->name; p++) {
    if(p > sigtable + 1)
      buffer_putspace(fd_out->w);
    buffer_puts(fd_out->w, p->name);
  }
  buffer_putnlflush(fd_out->w);
  return 0;
}

int
builtin_kill(int argc, char* argv[]) {
  int sig = SIGTERM;
  int i = 1;
  int ret = 0;

  if(argc > 1 && argv[1][0] == '-' && argv[1][1]) {
    if(argv[1][1] == 'l' && !argv[1][2]) {
      /* -l with optional argument */
      if(argc == 2) {
        /* No argument: list all signals */
        return kill_list();
      }
      /* Has argument: translate exit status or signal number to name */
      int n;
      if(!scan_int(argv[2], &n) || n < 0 || n > 128 + 31) {
        return builtin_errmsg(argv, argv[2], "invalid signal specification");
      }
      /* If n > 128, it's an exit status (128 + signal_number) */
      if(n > 128)
        n -= 128;
      const char* name = sig_name(n);
      if(!name) {
        return builtin_errmsg(argv, argv[2], "invalid signal specification");
      }
      buffer_puts(fd_out->w, name);
      buffer_putnlflush(fd_out->w);
      return 0;
    }

    if(argv[1][1] == 's' && !argv[1][2]) {
      if(argc < 3)
        return builtin_errmsg(argv, "-s", "option requires an argument");
      if((sig = kill_signum(argv[2])) < 0)
        return builtin_errmsg(argv, argv[2], "invalid signal specification");
      i = 3;
    } else {
      if((sig = kill_signum(&argv[1][1])) < 0)
        return builtin_errmsg(argv, argv[1], "invalid signal specification");
      i = 2;
    }
  }

  if(i >= argc)
    return builtin_errmsg(argv, "too few arguments", NULL);

  for(; i < argc; i++) {
    char* target = argv[i];

    if(target[0] == '%') {
      if(kill_job(argv, target, sig))
        ret = 1;

      continue;
    }

    {
      pid_t pid = 0;
      int n = 0;

      if(!target[0] || scan_int(target, &n) != str_len(target)) {
        builtin_errmsg(argv, target, "arguments must be process or job IDs");
        ret = 1;
        continue;
      }

      pid = n;

      if(kill(pid, sig) == -1) {
        builtin_error(argv, target);
        ret = 1;
      }
    }
  }

  return ret;
}
