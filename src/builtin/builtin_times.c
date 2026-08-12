#include "../builtin.h"
#include "../fdtable.h"
#include "../../lib/buffer.h"
#include "../../lib/fmt.h"
#include <sys/times.h>
#include <unistd.h>

/* print a time value in XmY.YYYYYYs format (minutes, seconds with 6 decimal places)
 * using buffer functions, no printf */
static void
print_time(clock_t ticks) {
  long clk_tck = sysconf(_SC_CLK_TCK);
  unsigned long total_microsecs;
  unsigned long minutes, seconds, microsecs;
  char buf[FMT_ULONG];
  size_t n, i;

  /* Convert ticks to microseconds */
  total_microsecs = (ticks * 1000000UL) / clk_tck;

  /* Extract minutes, seconds, microseconds */
  minutes = total_microsecs / 60000000UL;
  total_microsecs %= 60000000UL;
  seconds = total_microsecs / 1000000UL;
  microsecs = total_microsecs % 1000000UL;

  /* Print: minutes "m" seconds "." microsecs "s" */
  buffer_putulong(fd_out->w, minutes);
  buffer_puts(fd_out->w, "m");
  buffer_putulong(fd_out->w, seconds);
  buffer_puts(fd_out->w, ".");

  /* Print microsecs with leading zeros (always 6 digits) */
  n = fmt_ulong(buf, microsecs);
  for(i = n; i < 6; i++)
    buffer_putc(fd_out->w, '0');
  buffer_put(fd_out->w, buf, n);
  buffer_puts(fd_out->w, "s");
}

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
  struct tms buf;

  (void)argc;
  (void)argv;

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

  return 0;
}
