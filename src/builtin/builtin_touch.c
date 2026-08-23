#include "../builtin.h"
#include "../fdtable.h"
#include "../../lib/shell.h"
#include "../../lib/scan.h"
#include "../../lib/str.h"
#include "../../lib/byte.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <utime.h>
#include <unistd.h>

const char help_touch[] =
    "    Change file access and modification times, creating the file\n"
    "    if it doesn't already exist.\n"
    "\n"
    "    -a                     change only the access time\n"
    "    -m                     change only the modification time\n"
    "    -d, --date=DATE        use DATE instead of now ('@EPOCH',\n"
    "                           'YYYY-MM-DD', or 'YYYY-MM-DD hh:mm[:ss]')\n"
    "    -f                     ignored (compatibility)\n"
    "    -r, --reference=FILE   use FILE's times instead of now\n"
    "    -t STAMP               use [[CC]YY]MMDDhhmm[.ss] instead of now\n"
    "    --time=WORD            with -d/-t/-r: WORD 'access'/'atime'/'use'\n"
    "                           means -a, 'modify'/'mtime' means -m\n"
    "    file                   file(s) to touch\n";

/* removes 'n' argv slots starting at 'i', shifting the rest down;
 * 'argc' is adjusted in place.
 * ----------------------------------------------------------------------- */
static void
remove_args(char* argv[], int* argc, int i, int n) {
  byte_copyr(&argv[i], (*argc - i - n) * sizeof(char*), &argv[i + n]);
  *argc -= n;
}

/* pulls "NAME VALUE" or "NAME=VALUE" out of argv, wherever it
 * appears, returning VALUE, NULL if absent, or (char*)-1 if NAME was
 * given without a value.
 * ----------------------------------------------------------------------- */
static char*
extract_longopt(char* argv[], int* argc, const char* name) {
  size_t namelen = str_len(name);
  int i;

  for(i = 1; i < *argc; i++) {
    if(str_equal(argv[i], name)) {
      char* val = argv[i + 1];

      if(!val)
        return (char*)-1;

      remove_args(argv, argc, i, 2);
      return val;
    }

    if(!str_diffn(argv[i], name, namelen) && argv[i][namelen] == '=') {
      char* val = argv[i] + namelen + 1;

      remove_args(argv, argc, i, 1);
      return val;
    }
  }

  return NULL;
}

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
    if(s[len + 1] < '0' || s[len + 1] > '9' || s[len + 2] < '0' || s[len + 2] > '9' ||
       s[len + 3] != '\0')
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

/* parses touch --date's "@EPOCH", "YYYY-MM-DD", or
 * "YYYY-MM-DD[T ]hh:mm[:ss]" into an absolute time_t. This is not a
 * general natural-language date parser -- just these fixed forms.
 * Returns 0 on success, -1 if 's' isn't a recognized date.
 * ----------------------------------------------------------------------- */
static int
touch_parse_date(const char* s, time_t* out) {
  struct tm tm;
  unsigned long y, mo, d, h = 0, mi = 0, se = 0;
  size_t n;

  if(s[0] == '@') {
    unsigned long v;

    n = scan_ulong(s + 1, &v);

    if(!n || s[1 + n] != '\0')
      return -1;

    *out = (time_t)v;
    return 0;
  }

  n = scan_ulong(s, &y);

  if(n != 4 || s[n] != '-')
    return -1;
  s += n + 1;

  n = scan_ulong(s, &mo);

  if(n != 2 || s[n] != '-')
    return -1;
  s += n + 1;

  n = scan_ulong(s, &d);

  if(n != 2)
    return -1;
  s += n;

  if(*s == 'T' || *s == ' ') {
    s++;
    n = scan_ulong(s, &h);

    if(n != 2 || s[n] != ':')
      return -1;
    s += n + 1;

    n = scan_ulong(s, &mi);

    if(n != 2)
      return -1;
    s += n;

    if(*s == ':') {
      s++;
      n = scan_ulong(s, &se);

      if(n != 2)
        return -1;
      s += n;
    }
  }

  if(*s != '\0')
    return -1;

  byte_zero(&tm, sizeof(tm));
  tm.tm_year = (int)y - 1900;
  tm.tm_mon = (int)mo - 1;
  tm.tm_mday = (int)d;
  tm.tm_hour = (int)h;
  tm.tm_min = (int)mi;
  tm.tm_sec = (int)se;
  tm.tm_isdst = -1;

  *out = mktime(&tm);
  return *out == (time_t)-1 ? -1 : 0;
}

int
builtin_touch(int argc, char* argv[]) {
  int c, opt_a = 0, opt_m = 0, ret = 0, i, both;
  char *date_arg, *ref_arg, *time_arg, *stamp_arg = NULL;
  time_t requested = 0;
  int have_requested = 0;
  struct stat rst;

  date_arg = extract_longopt(argv, &argc, "--date");
  ref_arg = extract_longopt(argv, &argc, "--reference");
  time_arg = extract_longopt(argv, &argc, "--time");

  if(date_arg == (char*)-1 || ref_arg == (char*)-1 || time_arg == (char*)-1) {
    builtin_errmsg(argv, "option requires an argument", NULL);
    return 1;
  }

  while((c = shell_getopt(argc, argv, "amfd:r:t:")) > 0) {
    switch(c) {
      case 'a': opt_a = 1; break;
      case 'm': opt_m = 1; break;
      case 'f': break;
      case 'd': date_arg = shell_optarg; break;
      case 'r': ref_arg = shell_optarg; break;
      case 't': stamp_arg = shell_optarg; break;
      default: builtin_invopt(argv); return 1;
    }
  }

  if(time_arg) {
    if(str_equal(time_arg, "atime") || str_equal(time_arg, "access") ||
       str_equal(time_arg, "use"))
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
    if(touch_parse_date(date_arg, &requested) == -1) {
      builtin_errmsg(argv, date_arg, "invalid date");
      return 1;
    }

    have_requested = 1;
  } else if(stamp_arg) {
    struct tm tm;

    if(touch_parse_stamp(stamp_arg, &tm) == -1 || (requested = mktime(&tm)) == (time_t)-1) {
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
      int fd = open(path, O_WRONLY | O_CREAT, 0666);

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
