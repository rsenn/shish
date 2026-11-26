#include "builtin_config.h"

#if BUILTIN_STAT

#include "../../builtin.h"
#include "../../fdtable.h"
#include "../../../lib/fmt.h"
#include "../../../lib/scan.h"
#include "../../../lib/str.h"
#include "../../../lib/stralloc.h"
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>

const char help_stat[] = "    Display file status.\n"
                         "\n"
                         "    -L              follow symbolic links\n"
                         "    -c format       use FORMAT instead of the default; conversions:\n"
                         "                    %n name  %N quoted name  %s size  %b blocks  %o io block\n"
                         "                    %a octal mode  %A mode string  %f raw mode (hex)  %F type\n"
                         "                    %u %g owner/group id  %U %G owner/group name\n"
                         "                    %d %D device (decimal, hex)  %i inode  %h links\n"
                         "                    %x %y %z access/modify/change time  %X %Y %Z as seconds\n"
                         "                    %% a percent sign; flags - and 0 and a width are allowed\n"
                         "\n"
                         "    file            file(s) to examine\n";

static const char default_format[] = "  File: %N\n"
                                     "  Size: %-10s\tBlocks: %-10b IO Block: %-6o %F\n"
                                     "Device: %Dh/%dd\tInode: %-10i  Links: %h\n"
                                     "Access: (%04a/%A)  Uid: (%5u/%8U)   Gid: (%5g/%8G)\n"
                                     "Access: %x\n"
                                     "Modify: %y\n"
                                     "Change: %z\n";

static void
cat_num(stralloc* t, uint64 v) {
  char b[FMT_ULONG];

  stralloc_catb(t, b, fmt_ulonglong(b, v));
}

static void
cat_2(stralloc* t, unsigned long v, size_t w) {
  char b[FMT_ULONG];

  stralloc_catb(t, b, fmt_ulong0(b, v, w));
}

/* "yyyy-mm-dd HH:MM:SS.NNNNNNNNN +ZZZZ" */
static void
cat_stamp(stralloc* t, const struct timespec* ts) {
  struct tm tm;
  long off;

  localtime_r(&ts->tv_sec, &tm);
  cat_num(t, (uint64)tm.tm_year + 1900);
  stralloc_catb(t, "-", 1);
  cat_2(t, (unsigned long)tm.tm_mon + 1, 2);
  stralloc_catb(t, "-", 1);
  cat_2(t, (unsigned long)tm.tm_mday, 2);
  stralloc_catb(t, " ", 1);
  cat_2(t, (unsigned long)tm.tm_hour, 2);
  stralloc_catb(t, ":", 1);
  cat_2(t, (unsigned long)tm.tm_min, 2);
  stralloc_catb(t, ":", 1);
  cat_2(t, (unsigned long)tm.tm_sec, 2);
  stralloc_catb(t, ".", 1);
  cat_2(t, (unsigned long)ts->tv_nsec, 9);
  off = tm.tm_gmtoff;
  stralloc_catb(t, off < 0 ? " -" : " +", 2);

  if(off < 0)
    off = -off;

  cat_2(t, (unsigned long)(off / 3600 * 100 + off / 60 % 60), 4);
}

/* "-rwxr-xr-x", with the type letter first */
static void
cat_mode(stralloc* t, mode_t m) {
  char b[10];
  int i;

  for(i = 0; i < 9; i++)
    b[i] = (m & (0400 >> i)) ? "rwxrwxrwx"[i] : '-';

  if(m & S_ISUID)
    b[2] = b[2] == 'x' ? 's' : 'S';

  if(m & S_ISGID)
    b[5] = b[5] == 'x' ? 's' : 'S';

  if(m & S_ISVTX)
    b[8] = b[8] == 'x' ? 't' : 'T';

  stralloc_catb(t, S_ISDIR(m) ? "d" : S_ISLNK(m) ? "l" : S_ISBLK(m) ? "b" : S_ISCHR(m) ? "c" : S_ISFIFO(m) ? "p" : S_ISSOCK(m) ? "s" : "-", 1);
  stralloc_catb(t, b, 9);
}

static void
cat_type(stralloc* t, const struct stat* st) {
  mode_t m = st->st_mode;

  stralloc_cats(t,
                S_ISREG(m)    ? (st->st_size ? "regular file" : "regular empty file")
                : S_ISDIR(m)  ? "directory"
                : S_ISLNK(m)  ? "symbolic link"
                : S_ISBLK(m)  ? "block special file"
                : S_ISCHR(m)  ? "character special file"
                : S_ISFIFO(m) ? "fifo"
                : S_ISSOCK(m) ? "socket"
                              : "weird file");
}

/* owner or group name, or UNKNOWN */
static void
cat_name(stralloc* t, const char* file, unsigned long id) {
  if(!id_name(file, id, t))
    stralloc_cats(t, "UNKNOWN");
}

/* the text of one conversion; 0 if c is not one */
static int
convert(stralloc* t, int c, const char* path, const struct stat* st) {
  char link[1024];
  ssize_t r;

  switch(c) {
    case 'n': stralloc_cats(t, path); break;
    case 'N':
      stralloc_cats(t, "'");
      stralloc_cats(t, path);
      stralloc_cats(t, "'");

      if(S_ISLNK(st->st_mode) && (r = readlink(path, link, sizeof(link))) > 0) {
        stralloc_cats(t, " -> '");
        stralloc_catb(t, link, (size_t)r);
        stralloc_cats(t, "'");
      }

      break;
    case 's': cat_num(t, (uint64)st->st_size); break;
    case 'b': cat_num(t, (uint64)st->st_blocks); break;
    case 'B': cat_num(t, 512); break;
    case 'o': cat_num(t, (uint64)st->st_blksize); break;
    case 'd': cat_num(t, (uint64)st->st_dev); break;
    case 'D': {
      char b[FMT_XLONG];

      stralloc_catb(t, b, fmt_xlonglong(b, (uint64)st->st_dev));
      break;
    }
    case 'i': cat_num(t, (uint64)st->st_ino); break;
    case 'h': cat_num(t, (uint64)st->st_nlink); break;
    case 'u': cat_num(t, (uint64)st->st_uid); break;
    case 'g': cat_num(t, (uint64)st->st_gid); break;
    case 'U': cat_name(t, "/etc/passwd", st->st_uid); break;
    case 'G': cat_name(t, "/etc/group", st->st_gid); break;
    case 'a': {
      char b[FMT_ULONG];

      stralloc_catb(t, b, fmt_8long(b, st->st_mode & 07777));
      break;
    }
    case 'A': cat_mode(t, st->st_mode); break;
    case 'f': {
      char b[FMT_XLONG];

      stralloc_catb(t, b, fmt_xlonglong(b, st->st_mode));
      break;
    }
    case 'F': cat_type(t, st); break;
    case 'x': cat_stamp(t, &st->st_atim); break;
    case 'y': cat_stamp(t, &st->st_mtim); break;
    case 'z': cat_stamp(t, &st->st_ctim); break;
    case 'X': cat_num(t, (uint64)st->st_atim.tv_sec); break;
    case 'Y': cat_num(t, (uint64)st->st_mtim.tv_sec); break;
    case 'Z': cat_num(t, (uint64)st->st_ctim.tv_sec); break;
    case '%': stralloc_catb(t, "%", 1); break;
    default: return 0;
  }

  return 1;
}

/* the format applied to one file; unknown conversions are copied as they are */
static void
put_format(buffer* o, const char* f, const char* path, const struct stat* st, stralloc* t) {
  for(; *f; f++) {
    unsigned long width = 0;
    int left = 0, zero = 0;

    if(*f != '%') {
      buffer_putc(o, *f);
      continue;
    }

    for(f++; *f == '-' || *f == '0'; f++) {
      if(*f == '-')
        left = 1;
      else
        zero = 1;
    }

    f += scan_ulong(f, &width);

    if(!*f)
      break;

    stralloc_zero(t);

    if(!convert(t, *f, path, st)) {
      buffer_putc(o, '%');
      buffer_putc(o, *f);
      continue;
    }

    for(; !left && t->len < width; width--)
      buffer_putc(o, zero ? '0' : ' ');

    buffer_put(o, t->s, t->len);

    for(; left && t->len < width; width--)
      buffer_putc(o, ' ');
  }
}

int
builtin_stat(int argc, char* argv[]) {
  struct stat st;
  stralloc t = {0};
  const char* format = 0;
  int c, i, deref = 0, failed = 0;

  while((c = shell_getopt(argc, argv, "Lc:")) > 0) {
    switch(c) {
      case 'L': deref = 1; break;
      case 'c': format = shell_optarg; break;
      default: return builtin_invopt(argv);
    }
  }

  if(shell_optind == argc)
    return builtin_errmsg(argv, "operand", "missing file");

  for(i = shell_optind; i < argc; i++) {
    if((deref ? stat(argv[i], &st) : lstat(argv[i], &st)) == -1) {
      builtin_error(argv, argv[i]);
      failed = 1;
      continue;
    }

    put_format(fd_out->w, format ? format : default_format, argv[i], &st, &t);

    if(format)
      buffer_putc(fd_out->w, '\n');

    buffer_flush(fd_out->w);
  }

  stralloc_free(&t);
  return failed;
}

#endif /* BUILTIN_STAT */
