#include "../builtin.h"
#include "../fdtable.h"
#include "../../lib/buffer.h"
#include "../../lib/fmt.h"
#include "../../lib/windoze.h"

#if WINDOWS_NATIVE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/times.h>
#include <unistd.h>
#endif

/* print a time value given in microseconds as XmY.YYYYYYs (minutes,
 * seconds with 6 decimal places), using buffer functions, no printf */
static void
print_microsecs(unsigned long total_microsecs) {
  unsigned long minutes, seconds, microsecs;
  char buf[FMT_ULONG];
  size_t n, i;

  minutes = total_microsecs / 60000000UL;
  total_microsecs %= 60000000UL;
  seconds = total_microsecs / 1000000UL;
  microsecs = total_microsecs % 1000000UL;

  buffer_putulong(fd_out->w, minutes);
  buffer_puts(fd_out->w, "m");
  buffer_putulong(fd_out->w, seconds);
  buffer_puts(fd_out->w, ".");

  n = fmt_ulong(buf, microsecs);
  for(i = n; i < 6; i++)
    buffer_putc(fd_out->w, '0');
  buffer_put(fd_out->w, buf, n);
  buffer_puts(fd_out->w, "s");
}

#if WINDOWS_NATIVE
static void
print_filetime(const FILETIME* ft) {
  ULARGE_INTEGER u;

  u.LowPart = ft->dwLowDateTime;
  u.HighPart = ft->dwHighDateTime;

  /* FILETIME counts 100ns intervals */
  print_microsecs((unsigned long)(u.QuadPart / 10));
}
#else
static void
print_time(clock_t ticks) {
  long clk_tck = sysconf(_SC_CLK_TCK);

  print_microsecs((ticks * 1000000UL) / clk_tck);
}
#endif

/* times built-in
 *
 * ----------------------------------------------------------------------- */
const char help_times[] = "    Write accumulated user and system times to stdout.\n"
                          "\n"
                          "    The output format is:\n"
                          "        <user time> <system time>\n"
                          "        <user time for children> <system time for children>\n"
                          "\n"
                          "    Each time is printed as XmY.YYs (minutes, seconds with 2 decimal places).\n";

int
builtin_times(int argc, char* argv[]) {
  (void)argc;
  (void)argv;

#if WINDOWS_NATIVE
  {
    FILETIME creation, exit, kernel, user, zero = {0, 0};

    if(!GetProcessTimes(GetCurrentProcess(), &creation, &exit, &kernel, &user))
      return 1;

    /* Line 1: shell user and system times */
    print_filetime(&user);
    buffer_putspace(fd_out->w);
    print_filetime(&kernel);
    buffer_putnlflush(fd_out->w);

    /* Line 2: children user and system times -- Windows has no
       cumulative-child-CPU-time equivalent to wait4()/getrusage()
       without shish tracking every child itself, which it does not,
       so report zero rather than fabricate a number */
    print_filetime(&zero);
    buffer_putspace(fd_out->w);
    print_filetime(&zero);
    buffer_putnlflush(fd_out->w);
  }
#else
  {
    struct tms buf;

    if(times(&buf) == (clock_t)-1)
      return 1;

    /* Line 1: shell user and system times */
    print_time(buf.tms_utime);
    buffer_putspace(fd_out->w);
    print_time(buf.tms_stime);
    buffer_putnlflush(fd_out->w);

    /* Line 2: children user and system times */
    print_time(buf.tms_cutime);
    buffer_putspace(fd_out->w);
    print_time(buf.tms_cstime);
    buffer_putnlflush(fd_out->w);
  }
#endif

  return 0;
}
