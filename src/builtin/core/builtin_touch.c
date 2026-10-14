#include "builtin_config.h"

#if BUILTIN_TOUCH

#include "../../builtin.h"
#include "../../sh.h"
#include "../../../lib/date.h"
#include "../../fdtable.h"
#include "../../../lib/shell.h"
#include "../../../lib/scan.h"
#include "../../../lib/str.h"
#include "../../../lib/byte.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <utime.h>
#include <unistd.h>

const char help_touch[] = "    Change file access and modification times, creating the file\n"
                          "    if it doesn't already exist.\n"
                          "\n"
                          "    -a                     change only the access time\n"
                          "    -c                     do not create a file that does not exist\n"
                          "    -m                     change only the modification time\n"
                          "    -d, --date=DATE        use DATE instead of now ('@EPOCH',\n"
                          "                           'YYYY-MM-DD', or 'YYYY-MM-DD hh:mm[:ss]')\n"
                          "    -f                     ignored (compatibility)\n"
                          "    -r, --reference=FILE   use FILE's times instead of now\n"
                          "    -t STAMP               use [[CC]YY]MMDDhhmm[.ss] instead of now\n"
                          "    --time=WORD            with -d/-t/-r: WORD 'access'/'atime'/'use'\n"
                          "                           means -a, 'modify'/'mtime' means -m\n"
                          "    file                   file(s) to touch\n";

/* parses touch -t's "[[CC]YY]MMDDhhmm[.ss]" into 'tm' (local time).
 * Returns 0 on success, -1 if 's' isn't a valid stamp.
 * ----------------------------------------------------------------------- */
static int
touch_parse_stamp(const char* s, struct tm* tm) {
  char digits[13];
  size_t len = 0;
  int sec = 0, cc = -1, yy = -1;
  const char* p;

  while(len < 12 && s[len] >= '0' && s[len] <= '9') {
    digits[len] = s[len];
    len++;
  }

  if(s[len] == '.') {
    if(s[len + 1] < '0' || s[len + 1] > '9' || s[len + 2] < '0' || s[len + 2] > '9' || s[len + 3] != '\0')
      return -1;

    sec = (s[len + 1] - '0') * 10 + (s[len + 2] - '0');
  } else if(s[len] != '\0') {
    return -1;
  }

  if(len != 8 && len != 10 && len != 12)
    return -1;

  p = digits;

  if(len == 12) {
    cc = (p[0] - '0') * 10 + (p[1] - '0');
    yy = (p[2] - '0') * 10 + (p[3] - '0');
    p += 4;
  } else if(len == 10) {
    yy = (p[0] - '0') * 10 + (p[1] - '0');
    p += 2;
  }

  byte_zero(tm, sizeof(*tm));

  if(cc >= 0) {
    tm->tm_year = cc * 100 + yy - 1900;
  } else if(yy >= 0) {
    tm->tm_year = (yy <= 68 ? 2000 + yy : 1900 + yy) - 1900;
  } else {
    time_t now = time(NULL);
    struct tm* cur = localtime(&now);

    tm->tm_year = cur->tm_year;
  }

  tm->tm_mon = (p[0] - '0') * 10 + (p[1] - '0') - 1;
  tm->tm_mday = (p[2] - '0') * 10 + (p[3] - '0');
  tm->tm_hour = (p[4] - '0') * 10 + (p[5] - '0');
  tm->tm_min = (p[6] - '0') * 10 + (p[7] - '0');
  tm->tm_sec = sec;
  tm->tm_isdst = -1;
  return 0;
}

int
builtin_touch(int argc, char* argv[]) {
  int c, opt_a = 0, opt_m = 0, ret = 0, i, both, nocreate = 0;
  char *date_arg, *ref_arg, *time_arg, *stamp_arg = NULL;
  time_t requested = 0;
  int have_requested = 0;
  struct stat rst;

  date_arg = ref_arg = time_arg = NULL;

  while((c = shell_getopt_long(argc, argv, "acmfd:r:t:", "date:=d reference:=r time:=T no-create=c")) > 0) {
    switch(c) {
      case 'a': opt_a = 1; break;
      case 'c': nocreate = 1; break;
      case 'm': opt_m = 1; break;
      case 'f': break;
      case 'd': date_arg = shell_optarg; break;
      case 'r': ref_arg = shell_optarg; break;
      case 't': stamp_arg = shell_optarg; break;
      case 'T': time_arg = shell_optarg; break;
      default: return builtin_invopt(argv);
    }
  }

  if(time_arg) {
    if(str_equal(time_arg, "atime") || str_equal(time_arg, "access") || str_equal(time_arg, "use"))
      opt_a = 1;
    else if(str_equal(time_arg, "mtime") || str_equal(time_arg, "modify"))
      opt_m = 1;
    else {
      builtin_errmsg(argv, time_arg, "invalid argument for '--time'");
      return 1;
    }
  }

  if((date_arg != NULL) + (ref_arg != NULL) + (stamp_arg != NULL) > 1) {
    builtin_errmsg(argv, "cannot specify times from more than one source", NULL);
    return 1;
  }

  if(date_arg) {
    char* oldtz = sh_tz_enter(0);
    int bad = date_parse(date_arg, time(NULL), &requested) == -1;

    sh_tz_leave(oldtz);

    if(bad) {
      builtin_errmsg(argv, date_arg, "invalid date");
      return 1;
    }

    have_requested = 1;
  } else if(stamp_arg) {
    struct tm tm;
    char* oldtz = sh_tz_enter(0);
    int bad = touch_parse_stamp(stamp_arg, &tm) == -1 || (requested = mktime(&tm)) == (time_t)-1;

    sh_tz_leave(oldtz);

    if(bad) {
      builtin_errmsg(argv, stamp_arg, "invalid date format");
      return 1;
    }

    have_requested = 1;
  } else if(ref_arg) {
    if(stat(ref_arg, &rst) == -1) {
      builtin_error(argv, ref_arg);
      return 1;
    }
  }

  if(shell_optind >= argc) {
    builtin_errmsg(argv, "missing operand", NULL);
    return 1;
  }

  both = !(opt_a || opt_m);

  if(!ref_arg && !have_requested)
    requested = time(NULL);

  for(i = shell_optind; i < argc; i++) {
    char* path = argv[i];
    struct stat st;
    struct utimbuf ut;

    /* only create+restat when the file is actually missing -- opening
       an existing file here would risk bumping its atime before we
       get a chance to read the "leave this one alone" baseline. */
    if(stat(path, &st) == -1) {
      int fd;

      /* -c: a missing file is skipped, not an error */
      if(nocreate)
        continue;

      fd = open(path, O_WRONLY | O_CREAT, 0666);

      if(fd == -1 || close(fd) == -1 || stat(path, &st) == -1) {
        builtin_error(argv, path);
        ret = 1;
        continue;
      }
    }

    if(ref_arg) {
      ut.actime = rst.st_atime;
      ut.modtime = rst.st_mtime;
    } else {
      ut.actime = requested;
      ut.modtime = requested;
    }

    if(!(opt_a || both))
      ut.actime = st.st_atime;

    if(!(opt_m || both))
      ut.modtime = st.st_mtime;

    if(utime(path, &ut) == -1) {
      builtin_error(argv, path);
      ret = 1;
    }
  }

  return ret;
}
#endif /* BUILTIN_TOUCH */
