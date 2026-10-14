#include "builtin_config.h"

#if BUILTIN_DATE

#include "../../builtin.h"
#include "../../fdtable.h"
#include "../../sh.h"
#include "../../../lib/shell.h"
#include "../../../lib/stralloc.h"
#include "../../../lib/alloc.h"
#include "../../../lib/windoze.h"
#include "../../../lib/date.h"
#include "../../../lib/str.h"
#include "../../../lib/byte.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>

const char help_date[] = "    Print the date and time.\n"
                         "\n"
                         "    -u, --utc, --universal   use Coordinated Universal Time\n"
                         "    -d, --date=STRING        show STRING instead of now: '@EPOCH', now/today/yesterday/\n"
                         "                             tomorrow, 'YYYY-MM-DD[ hh:mm[:ss]]', 'hh:mm', '2 days ago'\n"
                         "    -r, --reference=FILE     show the modification time of FILE\n"
                         "    -I[FMT], --iso-8601[=FMT]  ISO 8601; FMT is date (default), hours, minutes, seconds\n"
                         "    -R, --rfc-email          RFC 5322 format\n"
                         "    --rfc-3339=FMT           RFC 3339; FMT is date, seconds\n"
                         "    +format         strftime() format; default \"%a %b %e %H:%M:%S %Z %Y\"\n"
                         "\n"
                         "    Setting the clock is not supported.\n";

/* strftime() into a growing buffer; an empty result is a valid one
 * ----------------------------------------------------------------------- */
static void
date_format(stralloc* out, const char* fmt, const struct tm* tm) {
  size_t n = 128, r;

  if(!*fmt)
    return;

  for(;;) {
    stralloc_ready(out, n);

    if((r = strftime(out->s, n, fmt, tm)) > 0 || n > 65536) {
      out->len = r;
      return;
    }

    n *= 4;
  }
}

int
builtin_date(int argc, char* argv[]) {
  int c, utc = 0, colon = 0;
  char *date_arg = 0, *ref_arg = 0; const char *iso = 0, *rfc3339 = 0, *fmt = "%a %b %e %H:%M:%S %Z %Y", *tz;
  char* oldtz = 0;
  time_t now;
  struct tm* tm;
  stralloc out;

  while((c = shell_getopt_long(argc, argv, "ud:r:I::R", "utc=u universal=u date:=d reference:=r iso-8601?=I rfc-email=R rfc-3339:=3")) > 0) {
    switch(c) {
      case 'u': utc = 1; break;
      case 'd': date_arg = shell_optarg; break;
      case 'r': ref_arg = shell_optarg; break;
      case 'I': iso = shell_optarg ? shell_optarg : "date"; break;
      case 'R': fmt = "%a, %d %b %Y %H:%M:%S %z"; break;
      case '3': rfc3339 = shell_optarg; break;
      default: return builtin_invopt(argv);
    }
  }

  if(argc - shell_optind > 1) {
    builtin_errmsg(argv, argv[shell_optind + 1], "extra operand");
    return 1;
  }

  if(iso || rfc3339) {
    const char* w = iso ? iso : rfc3339;

    if(str_equal(w, "date"))
      fmt = "%Y-%m-%d";
    else if(str_equal(w, "seconds") || (iso && str_equal(w, "ns"))) {
      fmt = iso ? "%Y-%m-%dT%H:%M:%S%z" : "%Y-%m-%d %H:%M:%S%z";
      colon = 1;
    } else if(iso && str_equal(w, "hours")) {
      fmt = "%Y-%m-%dT%H%z";
      colon = 1;
    } else if(iso && str_equal(w, "minutes")) {
      fmt = "%Y-%m-%dT%H:%M%z";
      colon = 1;
    } else {
      builtin_errmsg(argv, (char*)w, "invalid argument");
      return 2;
    }
  }

  if(argv[shell_optind]) {
    if(argv[shell_optind][0] != '+') {
      builtin_errmsg(argv, argv[shell_optind], "setting the clock is not supported");
      return 1;
    }

    fmt = argv[shell_optind] + 1;
  }

  now = time(0);

  if(ref_arg) {
    struct stat st;

    if(stat(ref_arg, &st) == -1) {
      builtin_error(argv, ref_arg);
      return 1;
    }

    now = st.st_mtime;
  }

  /* -u is "as if TZ=UTC0" */
  oldtz = sh_tz_enter(utc ? "UTC0" : 0);

  if(date_arg && date_parse(date_arg, now, &now) == -1) {
    builtin_errmsg(argv, (char*)date_arg, "invalid date");
    sh_tz_leave(oldtz);
    return 1;
  }

  tm = localtime(&now);

  if(!tm) {
    builtin_errmsg(argv, "time", "cannot convert");
    return 1;
  }

  stralloc_init(&out);
  date_format(&out, fmt, tm);

  /* "+0100" -> "+01:00" */
  if(colon && out.len >= 2) {
    stralloc_readyplus(&out, 1);
    byte_copyr(out.s + out.len - 1, 2, out.s + out.len - 2);
    out.s[out.len - 2] = ':';
    out.len++;
  }

  stralloc_catc(&out, '\n');
  buffer_putsa(fd_out->w, &out);
  buffer_flush(fd_out->w);
  stralloc_free(&out);

  sh_tz_leave(oldtz);
  return 0;
}
#endif /* BUILTIN_DATE */
