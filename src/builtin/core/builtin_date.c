#include "builtin_config.h"

#if BUILTIN_DATE

#include "../../builtin.h"
#include "../../fdtable.h"
#include "../../var.h"
#include "../../vartab.h"
#include "../../../lib/shell.h"
#include "../../../lib/stralloc.h"
#include "../../../lib/alloc.h"
#include "../../../lib/windoze.h"
#include <stdlib.h>
#include <time.h>

const char help_date[] = "    Print the date and time.\n"
                         "\n"
                         "    -u              use Coordinated Universal Time\n"
                         "    +format         strftime() format; default \"%a %b %e %H:%M:%S %Z %Y\"\n"
                         "\n"
                         "    Setting the clock is not supported.\n";

/* set (or, with v == 0, remove) TZ in the process environment
 * ----------------------------------------------------------------------- */
static void
date_settz(const char* v) {
#if WINDOWS_NATIVE
  stralloc sa;

  stralloc_init(&sa);
  stralloc_copys(&sa, "TZ=");
  stralloc_cats(&sa, v ? v : "");
  stralloc_nul(&sa);
  _putenv(sa.s);
  stralloc_free(&sa);
#else
  if(v)
    setenv("TZ", v, 1);
  else
    unsetenv("TZ");
#endif

  tzset();
}

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
  int c, utc = 0;
  const char *fmt = "%a %b %e %H:%M:%S %Z %Y", *tz;
  char* oldtz = 0;
  time_t now;
  struct tm* tm;
  stralloc out;

  while((c = shell_getopt(argc, argv, "u")) > 0) {
    switch(c) {
      case 'u': utc = 1; break;
      default: builtin_invopt(argv); return 1;
    }
  }

  if(argc - shell_optind > 1) {
    builtin_errmsg(argv, argv[shell_optind + 1], "extra operand");
    return 1;
  }

  if(argv[shell_optind]) {
    if(argv[shell_optind][0] != '+') {
      builtin_errmsg(argv, argv[shell_optind], "setting the clock is not supported");
      return 1;
    }

    fmt = argv[shell_optind] + 1;
  }

  now = time(0);

  /* libc reads TZ from the process environment, not from the shell's
     variables; -u is "as if TZ=UTC0" */
  {
    struct var* tzvar = var_search("TZ", 0);

    tz = utc ? "UTC0" : tzvar && !(tzvar->flags & V_UNSET) ? &tzvar->sa.s[tzvar->offset] : 0;

    if(getenv("TZ"))
      oldtz = str_dup(getenv("TZ"));

    date_settz(tz);
    tm = localtime(&now);
  }

  if(!tm) {
    builtin_errmsg(argv, "time", "cannot convert");
    return 1;
  }

  stralloc_init(&out);
  date_format(&out, fmt, tm);
  stralloc_catc(&out, '\n');
  buffer_putsa(fd_out->w, &out);
  buffer_flush(fd_out->w);
  stralloc_free(&out);

  date_settz(oldtz);

  if(oldtz)
    alloc_free(oldtz);

  return 0;
}
#endif /* BUILTIN_DATE */
