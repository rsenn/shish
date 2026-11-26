#include "builtin_config.h"

#if BUILTIN_UPTIME

#include "../../builtin.h"
#include "../../fdtable.h"
#include "../../../lib/open.h"
#include "../../../lib/scan.h"
#include "../../../lib/str.h"
#include "../../../lib/stralloc.h"
#include "../../../lib/fmt.h"
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <utmp.h>

const char help_uptime[] = "    Tell how long the system has been running.\n"
                           "\n"
                           "    -p              show uptime in pretty format\n"
                           "    -s              show system up since, as yyyy-mm-dd HH:MM:SS\n";

/* read a whole /proc file into sa, NUL-terminated; 0 or -1 */
static int
slurp(const char* path, stralloc* sa) {
  int fd;

  stralloc_zero(sa);

  if((fd = open_read(path)) == -1)
    return -1;

  if(readclose_append(fd, sa, 1024) < 0)
    return -1;

  stralloc_0(sa);
  return 0;
}

/* v as at least two digits, zero-padded */
static void
put_2(buffer* o, unsigned long v) {
  char buf[FMT_ULONG];

  buffer_put(o, buf, fmt_ulong0(buf, v, 2));
}

/* HH:MM:SS (clock) or yyyy-mm-dd HH:MM:SS (date) */
static void
put_time(buffer* o, time_t t, int date) {
  struct tm tm;

  localtime_r(&t, &tm);

  if(date) {
    buffer_putulong(o, (unsigned long)tm.tm_year + 1900);
    buffer_putc(o, '-');
    put_2(o, (unsigned long)tm.tm_mon + 1);
    buffer_putc(o, '-');
    put_2(o, (unsigned long)tm.tm_mday);
    buffer_putc(o, ' ');
  }

  put_2(o, (unsigned long)tm.tm_hour);
  buffer_putc(o, ':');
  put_2(o, (unsigned long)tm.tm_min);
  buffer_putc(o, ':');
  put_2(o, (unsigned long)tm.tm_sec);
}

/* "N unit(s)", preceded by ", " unless it is the first part */
static void
put_part(buffer* o, int* first, unsigned long n, const char* unit) {
  if(!n)
    return;

  if(!*first)
    buffer_puts(o, ", ");

  *first = 0;
  buffer_putulong(o, n);
  buffer_putspace(o);
  buffer_puts(o, unit);

  if(n != 1)
    buffer_putc(o, 's');
}

/* logged-in sessions in utmp */
static unsigned long
count_users(void) {
  struct utmp ut;
  unsigned long n = 0;
  int fd;

  if((fd = open_read("/var/run/utmp")) == -1)
    return 0;

  while(read(fd, &ut, sizeof(ut)) == (ssize_t)sizeof(ut))
    if(ut.ut_type == USER_PROCESS && ut.ut_pid > 0 && (!kill(ut.ut_pid, 0) || errno == EPERM))
      n++;

  close(fd);
  return n;
}

int
builtin_uptime(int argc, char* argv[]) {
  buffer* o = fd_out->w;
  stralloc sa = {0};
  unsigned long up = 0, days, hours, mins, users;
  time_t now = time(0);
  const char* p;
  int c, pretty = 0, since = 0, i;

  while((c = shell_getopt(argc, argv, "ps")) > 0) {
    switch(c) {
      case 'p': pretty = 1; break;
      case 's': since = 1; break;
      default: return builtin_invopt(argv);
    }
  }

  if(slurp("/proc/uptime", &sa) == -1)
    return builtin_error(argv, "/proc/uptime");

  scan_ulong(sa.s, &up);
  days = up / 86400;
  hours = up / 3600 % 24;
  mins = up / 60 % 60;

  if(since) {
    put_time(o, now - (time_t)up, 1);
    buffer_putnlflush(o);
    stralloc_free(&sa);
    return 0;
  }

  if(pretty) {
    buffer_puts(o, "up ");
    i = 1;
    put_part(o, &i, days, "day");
    put_part(o, &i, hours, "hour");
    put_part(o, &i, mins, "minute");

    if(i)
      buffer_puts(o, "0 minutes");

    buffer_putnlflush(o);
    stralloc_free(&sa);
    return 0;
  }

  buffer_putspace(o);
  put_time(o, now, 0);
  buffer_puts(o, " up ");

  if(days) {
    buffer_putulong(o, days);
    buffer_puts(o, days == 1 ? " day, " : " days, ");
  }

  if(days || hours) {
    if(hours < 10)
      buffer_putspace(o);

    buffer_putulong(o, hours);
    buffer_putc(o, ':');
    put_2(o, mins);
  } else {
    buffer_putulong(o, mins);
    buffer_puts(o, " min");
  }

  users = count_users();
  buffer_puts(o, ", ");
  buffer_putulong(o, users);
  buffer_puts(o, users == 1 ? " user" : " users");
  buffer_puts(o, ",  load average: ");

  if(slurp("/proc/loadavg", &sa) == 0) {
    p = sa.s;

    for(i = 0; i < 3; i++) {
      size_t n = str_chr(p, ' ');

      if(i)
        buffer_puts(o, ", ");

      buffer_put(o, p, n);
      p += n + 1;
    }
  }

  buffer_putnlflush(o);
  stralloc_free(&sa);
  return 0;
}

#endif /* BUILTIN_UPTIME */
