#include "../eval.h"
#include "../fdtable.h"
#include "../tree.h"
#include "../../lib/buffer.h"
#include "../../lib/fmt.h"
#include "../../lib/windoze.h"

#if !WINDOWS_NATIVE
#include <sys/times.h>
#include <unistd.h>

/* one report line, ms milliseconds:
 *   posix   "real 0.10"
 *   bash    "real<TAB>0m0.101s"
 * ----------------------------------------------------------------------- */
static void
time_line(buffer* b, const char* name, unsigned long ms, int posix) {
  char num[FMT_ULONG];
  unsigned long s = ms / 1000;

  buffer_puts(b, name);
  buffer_puts(b, posix ? " " : "\t");

  if(!posix) {
    buffer_put(b, num, fmt_ulong(num, s / 60));
    buffer_puts(b, "m");
    s %= 60;
  }

  buffer_put(b, num, fmt_ulong(num, s));
  buffer_puts(b, ".");
  buffer_put(b, num, fmt_ulong0(num, posix ? ms % 1000 / 10 : ms % 1000, posix ? 2 : 3));
  buffer_puts(b, posix ? "\n" : "s\n");
}

/* ticks to milliseconds, in 64 bits: a 32-bit unsigned long overflows */
static unsigned long
time_ms(clock_t ticks, unsigned long long hz) {
  return (unsigned long)((unsigned long long)ticks * 1000 / hz);
}
#endif

/* time [-p] pipeline: run the pipeline, then report real, user and system time on stderr.
 * The status is the pipeline's; "set -e" is left to the caller, so the report is printed first.
 * ----------------------------------------------------------------------- */
int
eval_time(struct eval* e, struct ntime* t) {
  int ret;
#if !WINDOWS_NATIVE
  struct tms a, b;
  clock_t ra, rb;
#ifdef __wasi__
  /* wasi-libc times() counts nanoseconds, not sysconf(_SC_CLK_TCK) ticks */
  unsigned long long hz = 1000000000ULL;
#else
  unsigned long long hz = (unsigned long long)sysconf(_SC_CLK_TCK);

  if(hz == 0)
    hz = 100;
#endif

  ra = times(&a);
  errexit_suppress++;
  ret = eval_tree(e, t->pipeline, 0);
  errexit_suppress--;
  rb = times(&b);

  if(!t->posix)
    buffer_puts(fd_err->w, "\n");

  time_line(fd_err->w, "real", time_ms(rb - ra, hz), t->posix);
  time_line(fd_err->w, "user", time_ms(b.tms_utime - a.tms_utime + b.tms_cutime - a.tms_cutime, hz), t->posix);
  time_line(fd_err->w, "sys", time_ms(b.tms_stime - a.tms_stime + b.tms_cstime - a.tms_cstime, hz), t->posix);
  buffer_flush(fd_err->w);
#else
  ret = eval_tree(e, t->pipeline, 0);
#endif
  return ret;
}
