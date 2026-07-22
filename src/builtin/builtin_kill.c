#include "../builtin.h"
#include "../job.h"
#include "../../lib/scan.h"
#include "../../lib/str.h"
#include "../../lib/sig.h"
#include <signal.h>

/* parse a "-signal" operand: either numeric ("-9") or a name
 * ("-TERM"/"-SIGTERM", case-insensitive, with or without the "SIG"
 * prefix -- sig_number() itself doesn't strip that, so it's stripped
 * here the same way sig_byname() does it internally). Returns the
 * signal number, or -1 if spec is neither a full number nor a
 * recognized name.
 * ----------------------------------------------------------------------- */
static int
kill_signum(const char* spec) {
  int n = 0;

  if(spec[0] && scan_int(spec, &n) == str_len(spec))
    return n;

  if(!str_case_diffn(spec, "SIG", 3))
    spec += 3;

  /* sig_number() returns 0 both for the real "EXIT" pseudo-signal and
     for a name it doesn't recognize -- disambiguate the two here so
     an unknown name is reported instead of silently acting like
     "kill -EXIT" (i.e. signal 0, "is this pid still alive?"). */
  n = sig_number(spec);

  if(n == 0 && str_case_diff(spec, "EXIT"))
    return -1;

  return n;
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
int
builtin_kill(int argc, char* argv[]) {
  int sig = SIGTERM;
  int i = 1;
  int ret = 0;

  if(argc > 1 && argv[1][0] == '-' && argv[1][1]) {
    if((sig = kill_signum(&argv[1][1])) < 0)
      return builtin_errmsg(argv, argv[1], "invalid signal specification");

    i = 2;
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
