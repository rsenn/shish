#include "../builtin.h"
#include "../fdtable.h"
#include "../../lib/scan.h"
#include <unistd.h>

const char help_sleep[] = "    Suspend execution for a number of seconds.\n"
                          "\n"
                          "    seconds         number of seconds to sleep; may be fractional\n"
                          "                    ('1.5') and end in a unit suffix: 's' seconds,\n"
                          "                    'm' minutes, 'h' hours, 'd' days\n";

/* parses "NUMBER[.FRACTION][smhd]" (seconds by default) into a whole-
 * second count plus a sub-second microsecond remainder. Returns 0 on
 * success, -1 if 's' isn't a valid time interval.
 * ----------------------------------------------------------------------- */
static int
sleep_parse(const char* s, unsigned long* whole, unsigned int* usec) {
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
    case 's': mult = 1.0; s++; break;
    case 'm': mult = 60.0; s++; break;
    case 'h': mult = 3600.0; s++; break;
    case 'd': mult = 86400.0; s++; break;
    default: return -1;
  }

  if(*s != '\0')
    return -1;

  for(i = 0; i < fraclen; i++)
    scale *= 10.0;

  seconds = ((double)intpart + (double)frac / scale) * mult;
  *whole = (unsigned long)seconds;
  *usec = (unsigned int)((seconds - (double)*whole) * 1000000.0 + 0.5);
  return 0;
}

int
builtin_sleep(int argc, char* argv[]) {
  unsigned long whole;
  unsigned int usec;

  if(argc < 2) {
    builtin_errmsg(argv, "missing operand", NULL);
    return 1;
  }

  if(argc > 2) {
    builtin_errmsg(argv, argv[2], "extra operand");
    return 1;
  }

  if(sleep_parse(argv[1], &whole, &usec) == -1) {
    builtin_errmsg(argv, argv[1], "invalid time interval");
    return 1;
  }

  if(whole)
    sleep((unsigned int)whole);

  if(usec)
    usleep(usec);

  return 0;
}
