#include "../../lib/uint64.h"
#include "../builtin.h"
#include "../fdtable.h"
#include "../../lib/shell.h"
#include "../../lib/stralloc.h"
#include "../../lib/str.h"
#include "../../lib/alloc.h"
#include "../../lib/fmt.h"
#include "../../lib/unix.h"
#include "config.h"
#include <dirent.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#if defined(HAVE_GETPWUID_R) || defined(HAVE_GETPWUID)
#include <pwd.h>
#endif
#if defined(HAVE_GETGRGID_R) || defined(HAVE_GETGRGID)
#include <grp.h>
#endif

static int
ls_name_cmp(const void* a, const void* b) {
  return str_diff(*(char* const*)a, *(char* const*)b);
}

/* appends a 10-char "drwxrwxrwx"-style mode string to b -- covers the
 * type bits this codebase's own S_IF* macros define (dir/char/block/
 * fifo/socket/symlink), '-' for a plain file, '?' for anything else.
 * ----------------------------------------------------------------------- */
static void
ls_put_mode(buffer* b, unsigned int mode) {
  static const char perm[9] = "rwxrwxrwx";
  char type;
  unsigned int i;

  switch(mode & S_IFMT) {
    case S_IFDIR: type = 'd'; break;
    case S_IFREG: type = '-'; break;
#ifdef S_IFLNK
    case S_IFLNK: type = 'l'; break;
#endif
#ifdef S_IFCHR
    case S_IFCHR: type = 'c'; break;
#endif
#ifdef S_IFBLK
    case S_IFBLK: type = 'b'; break;
#endif
#ifdef S_IFIFO
    case S_IFIFO: type = 'p'; break;
#endif
#ifdef S_IFSOCK
    case S_IFSOCK: type = 's'; break;
#endif
    default: type = '?'; break;
  }
  buffer_putc(b, type);

  for(i = 0; i < 9; i++)
    buffer_putc(b, (mode & (0400 >> i)) ? perm[i] : '-');
}

/* like buffer_putulong0() but for a uint64 -- right-pads with leading
 * spaces to 'pad' columns.
 * ----------------------------------------------------------------------- */
static void
ls_put_size(buffer* b, uint64 size, int pad) {
  char buf[FMT_ULONG];
  ssize_t n = fmt_ulonglong(buf, size);

  if(n < pad)
    buffer_putnspace(b, pad - n);

  buffer_put(b, buf, n);
}

/* prints a user name for 'uid', falling back to the numeric id when
 * no passwd lookup is available or the lookup fails.
 * ----------------------------------------------------------------------- */
static void
ls_put_user(buffer* b, unsigned long uid) {
#if defined(HAVE_GETPWUID_R)
  struct passwd pw, *result = NULL;
  char buf[1024];

  if(getpwuid_r((uid_t)uid, &pw, buf, sizeof(buf), &result) == 0 && result) {
    buffer_puts(b, pw.pw_name);
    return;
  }
#elif defined(HAVE_GETPWUID)
  struct passwd* pw = getpwuid((uid_t)uid);

  if(pw) {
    buffer_puts(b, pw->pw_name);
    return;
  }
#endif
  buffer_putulong(b, uid);
}

/* prints a group name for 'gid', falling back to the numeric id when
 * no group lookup is available or the lookup fails.
 * ----------------------------------------------------------------------- */
static void
ls_put_group(buffer* b, unsigned long gid) {
#if defined(HAVE_GETGRGID_R)
  struct group gr, *result = NULL;
  char buf[1024];

  if(getgrgid_r((gid_t)gid, &gr, buf, sizeof(buf), &result) == 0 && result) {
    buffer_puts(b, gr.gr_name);
    return;
  }
#elif defined(HAVE_GETGRGID)
  struct group* gr = getgrgid((gid_t)gid);

  if(gr) {
    buffer_puts(b, gr->gr_name);
    return;
  }
#endif
  buffer_putulong(b, gid);
}

/* prints one entry: "-l" long format (mode, link count, owner,
 * group, size) followed by 'name', or just 'name' alone. Always
 * lstat()s 'path' for the long format -- a symlink shows as itself
 * ('l', its own size), not the file it points to. Owner/group print
 * by name when a passwd/group lookup is available, numeric otherwise.
 * ----------------------------------------------------------------------- */
static int
ls_print(char* argv[], const char* path, const char* name, int long_fmt) {
  struct stat st;

  if(lstat(path, &st) == -1)
    return builtin_error(argv, (char*)path);

  if(long_fmt) {
    ls_put_mode(fd_out->w, st.st_mode);
    buffer_putspace(fd_out->w);
    buffer_putulong0(fd_out->w, st.st_nlink, 3);
    buffer_putspace(fd_out->w);
    ls_put_user(fd_out->w, (unsigned long)st.st_uid);
    buffer_putspace(fd_out->w);
    ls_put_group(fd_out->w, (unsigned long)st.st_gid);
    buffer_putspace(fd_out->w);
    ls_put_size(fd_out->w, (uint64)st.st_size, 6);
    buffer_putspace(fd_out->w);
  }

  buffer_puts(fd_out->w, name);
  buffer_putnlflush(fd_out->w);
  return 0;
}

/* lists a directory's sorted contents (skipping dotfiles unless
 * 'all'), each printed via ls_print() against "path/name".
 * ----------------------------------------------------------------------- */
static int
ls_dir(char* argv[], const char* path, int all, int long_fmt) {
  DIR* dp;
  struct dirent* de;
  char** names = NULL;
  unsigned int n = 0, nalloc = 0, i;
  stralloc full;
  int ret = 0;

  if(!(dp = opendir(path)))
    return builtin_error(argv, (char*)path);

  while((de = readdir(dp))) {
    if(!str_diff(de->d_name, ".") || !str_diff(de->d_name, ".."))
      continue;

    if(!all && de->d_name[0] == '.')
      continue;

    if(n >= nalloc) {
      nalloc = nalloc ? nalloc * 2 : 16;
      names = alloc_re(names, nalloc * sizeof(char*));
    }

    names[n++] = str_dup(de->d_name);
  }

  closedir(dp);
  qsort(names, n, sizeof(char*), ls_name_cmp);

  stralloc_init(&full);

  for(i = 0; i < n; i++) {
    stralloc_copys(&full, path);

    if(full.len && full.s[full.len - 1] != '/')
      stralloc_catc(&full, '/');

    stralloc_cats(&full, names[i]);
    stralloc_nul(&full);

    if(ls_print(argv, full.s, names[i], long_fmt))
      ret = 1;

    alloc_free(names[i]);
  }

  stralloc_free(&full);
  alloc_free(names);
  return ret;
}

const char help_ls[] = "    List directory contents.\n"
                       "\n"
                       "    -a              include entries starting with '.'\n"
                       "    -d              list directories themselves, not their contents\n"
                       "    -l              long format: mode, links, owner, group, size\n"
                       "    -1              one entry per line (default)\n"
                       "    file            file or directory to list; defaults to '.'\n";

int
builtin_ls(int argc, char* argv[]) {
  int c, all = 0, dirs_only = 0, long_fmt = 0, ret = 0;
  char* single_dot[] = {".", NULL};
  char** args;
  int nargs, i;

  while((c = shell_getopt(argc, argv, "adl1")) > 0) {
    switch(c) {
      case 'a': all = 1; break;
      case 'd': dirs_only = 1; break;
      case 'l': long_fmt = 1; break;
      case '1': break;
      default: builtin_invopt(argv); return 1;
    }
  }

  args = &argv[shell_optind];
  nargs = argc - shell_optind;

  if(!nargs) {
    args = single_dot;
    nargs = 1;
  }

  for(i = 0; i < nargs; i++) {
    struct stat st;
    char* path = args[i];

    if(!dirs_only && stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
      if(nargs > 1) {
        buffer_puts(fd_out->w, path);
        buffer_puts(fd_out->w, ":");
        buffer_putnlflush(fd_out->w);
      }

      if(ls_dir(argv, path, all, long_fmt))
        ret = 1;

      if(nargs > 1 && i + 1 < nargs)
        buffer_putnlflush(fd_out->w);

      continue;
    }

    if(ls_print(argv, path, path, long_fmt))
      ret = 1;
  }

  return ret;
}
