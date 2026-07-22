#include "../builtin.h"
#include "../job.h"
#include "../fdtable.h"
#include "../../lib/wait.h"
#include <signal.h>

/* jobs builtin: list every job the shell is currently tracking
 * (backgrounded, stopped, or just-finished) -- one "[id] Status
 * command" line each, via the same job_print() "fg"/"bg" also use for
 * the "[N]+ Done ..."/"[N]+ Stopped ..." banners.
 * ----------------------------------------------------------------------- */
int
builtin_jobs(int argc, char* argv[]) {
  struct job* j;

  for(j = job_list; j; j = j->next)
    job_print(j, fd_out->w);

  /* every job was just listed above, including any that had already
     finished ("Done") -- silently (print=false) drop those now, we
     don't need to announce them a second time */
  job_clean(false);

  return 0;
}

/* shared by fg and bg: resolve a "%N"/pid/bare-id operand to a job,
 * or -- with no operand at all -- fall back to the current job ("%+").
 * Reports its own "no such job" error so callers can just bail out on
 * a NULL return.
 * ----------------------------------------------------------------------- */
static struct job*
jobs_resolve(char* argv[], const char* spec) {
  struct job* j = spec ? job_find(spec) : job_current();

  if(!j)
    builtin_errmsg(argv, spec ? (char*)spec : "current", "no such job");

  return j;
}

/* make 'j' the current job ("%+"), the same bookkeeping job_new() does
 * for a freshly created job and job_free() does when the old current
 * job goes away -- fg/bg need it too, so that a following bare
 * "fg"/"bg"/"%%" targets whichever job was just brought to the front,
 * not whatever happened to be current before.
 * ----------------------------------------------------------------------- */
static void
job_setcurrent(struct job* j) {
  struct job** jp;

  for(jp = &job_list; *jp; jp = &(*jp)->next) {
    if(*jp == j) {
      job_pointer = jp;
      break;
    }
  }
}

/* send SIGCONT to a job's process group and un-stick job_wait()'s
 * bookkeeping for it. job_signal() recorded a WIFSTOPPED status for
 * whichever proc(s) actually stopped, and nothing clears that on its
 * own; job_wait()'s stopped/remaining scan reads procs[].status
 * directly, so without resetting a stopped entry back to -1 here, a
 * job we just resumed would still look stopped to it (fg would
 * immediately "finish" waiting without the job having done anything,
 * and bg would leave job_stopped() reporting true forever). Only
 * touches entries that were actually WIFSTOPPED -- a proc that had
 * already exited must keep its real exit status.
 * ----------------------------------------------------------------------- */
static void
job_resume(struct job* j) {
  size_t p;

  if(j->pgrp)
    killpg(j->pgrp, SIGCONT);

  for(p = 0; p < j->nproc; p++)
    if(j->procs[p].status != -1 && WAIT_IF_STOPPED(j->procs[p].status))
      j->procs[p].status = -1;
}

/* fg builtin: bring one job into the foreground and wait for it,
 * resuming it first (SIGCONT) if it had been stopped.
 * ----------------------------------------------------------------------- */
int
builtin_fg(int argc, char* argv[]) {
  struct job* j;
  int status = 0;

  /* unlike bg, fg only ever moves *one* job to the foreground at a
     time -- there's only one terminal to hand over */
  if(argc > 2) {
    builtin_errmsg(argv, "fg", "too many arguments");
    return 1;
  }

  if(!(j = jobs_resolve(argv, argc == 2 ? argv[1] : NULL)))
    return 1;

  job_setcurrent(j);

  /* echo the command being resumed, same as a real shell -- the job
     was referred to by number/pid, so remind the user what that
     actually is before it starts producing its own output */
  buffer_puts(fd_out->w, j->command ? j->command : "(null)");
  buffer_putnlflush(fd_out->w);

  /* hand the terminal over before waking the job back up, so it isn't
     racing the shell for input the instant it resumes (a no-op, same
     as SIGCONT below, if the job was already running rather than
     stopped) */
  if(j->pgrp)
    job_foreground(j);

  job_resume(j);

  /* this is now a foreground wait, not a backgrounded one -- clear
     bgnd so job_wait() doesn't also print a redundant "Done" banner
     once it finishes; the line above already told the user what's
     running, and they're watching it synchronously either way */
  j->bgnd = 0;

  job_wait(j, 0, &status);

  return WAIT_IF_EXITED(status) ? WAIT_EXITSTATUS(status) : 0;
}

/* bg builtin: resume one or more *stopped* jobs, left running in the
 * background (no terminal handover, no waiting).
 * ----------------------------------------------------------------------- */
int
builtin_bg(int argc, char* argv[]) {
  int i, ret = 0;
  /* "bg" alone acts on the current job; "bg %1 %2" can resume several
     at once (unlike fg, nothing here needs exclusive terminal access) */
  int n = argc > 1 ? argc - 1 : 1;

  for(i = 0; i < n; i++) {
    const char* spec = argc > 1 ? argv[i + 1] : NULL;
    struct job* j = jobs_resolve(argv, spec);

    if(!j) {
      ret = 1;
      continue;
    }

    if(!job_stopped(j)) {
      builtin_errmsg(argv, spec ? (char*)spec : "current", "job already in background");
      ret = 1;
      continue;
    }

    job_setcurrent(j);
    job_resume(j);
    j->bgnd = 1;

    job_banner(j, fd_out->w, JOB_BGRESUME);
  }

  return ret;
}
