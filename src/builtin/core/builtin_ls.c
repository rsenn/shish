#include "builtin_config.h"

#if BUILTIN_LS

#include "../../../lib/uint64.h"
#include "../../builtin.h"
#include "../../fdtable.h"
#include "../../../lib/shell.h"
#include "../../../lib/stralloc.h"
#include "../../../lib/str.h"
#include "../../../lib/alloc.h"
#include "../../../lib/byte.h"
#include "../../../lib/fmt.h"
#include "../../../lib/unix.h"
#include "config.h"
#include <dirent.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#if defined(HAVE_GETPWUID_R) || defined(HAVE_GETPWUID)
#include <pwd.h>
#endif
#if defined(HAVE_GETGRGID_R) || defined(HAVE_GETGRGID)
#include <grp.h>
#endif

/* what the -a/-A/-R/... flags asked for */
struct ls_opts {
  unsigned all : 1;      /* -a: include ".", ".." and dotfiles */
  unsigned almost : 1;   /* -A: dotfiles but not "." and ".." */
  unsigned dirs : 1;     /* -d */
  unsigned unsorted : 1; /* -f */
  unsigned long_fmt : 1; /* -l */
  unsigned recurse : 1;  /* -R */
  unsigned reverse : 1;  /* -r */
  unsigned by_time : 1;  /* -t */
  unsigned by_size : 1;  /* -S */
  unsigned slash : 1;    /* -p */
  unsigned classify : 1; /* -F */
  unsigned inode : 1;    /* -i */
};

/* one directory entry plus what lstat() said about it */
struct ls_ent {
  char* name;
  char* path;
  struct stat st;
};

static struct ls_opts ls_o;

/* name order, or -t newest first, or -S largest first; -r reverses it.
 * Ties fall back to the name, so the order is always total.
 * ----------------------------------------------------------------------- */
static int
ls_cmp(const void* a, const void* b) {
  const struct ls_ent *x = a, *y = b;
  int r = 0;

  if(ls_o.by_size)
    r = x->st.st_size < y->st.st_size ? 1 : x->st.st_size > y->st.st_size ? -1 : 0;
  else if(ls_o.by_time)
    r = x->st.st_mtime < y->st.st_mtime ? 1 : x->st.st_mtime > y->st.st_mtime ? -1 : 0;

  if(!r)
    r = str_diff(x->name, y->name);

  return ls_o.reverse ? -r : r;
}

/* "-rwxr-xr-x" with s/S (setuid, setgid) and t/T (sticky) where set; the
 * type letter covers dir/char/block/fifo/socket/symlink, '?' for the rest.
 * ----------------------------------------------------------------------- */
static void
ls_put_mode(buffer* b, unsigned int mode) {
  char out[11];
  unsigned int i;
  static const struct {
    unsigned int bit;
    int at;
    char on, off;
  } sp[3] = {{04000, 3, 's', 'S'}, {02000, 6, 's', 'S'}, {01000, 9, 't', 'T'}};

  switch(mode & S_IFMT) {
    case S_IFDIR: out[0] = 'd'; break;
    case S_IFREG: out[0] = '-'; break;
#ifdef S_IFLNK
    case S_IFLNK: out[0] = 'l'; break;
#endif
#ifdef S_IFCHR
    case S_IFCHR: out[0] = 'c'; break;
#endif
#ifdef S_IFBLK
    case S_IFBLK: out[0] = 'b'; break;
#endif
#ifdef S_IFIFO
    case S_IFIFO: out[0] = 'p'; break;
#endif
#ifdef S_IFSOCK
    case S_IFSOCK: out[0] = 's'; break;
#endif
    default: out[0] = '?'; break;
  }

  for(i = 0; i < 9; i++)
    out[1 + i] = (mode & (0400 >> i)) ? "rwxrwxrwx"[i] : '-';

  for(i = 0; i < 3; i++)
    if(mode & sp[i].bit)
      out[sp[i].at] = out[sp[i].at] == 'x' ? sp[i].on : sp[i].off;

  buffer_put(b, out, 10);
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

/* "Mon DD HH:MM" for the last six months, "Mon DD  YYYY" for older (or future) files */
static void
ls_put_time(buffer* b, time_t t) {
  static const char mon[12][4] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  time_t now = time(NULL);
  struct tm* tm = localtime(&t);

  if(!tm) {
    buffer_puts(b, "?");
    return;
  }

  buffer_put(b, mon[tm->tm_mon % 12], 3);
  buffer_putspace(b);
  buffer_putulong0(b, tm->tm_mday, 2);
  buffer_putspace(b);

  if(t <= now && now - t < 15778476) {
    buffer_putulong0(b, tm->tm_hour, 2);
    buffer_putc(b, ':');
    buffer_putulong0(b, tm->tm_min, 2);
  } else {
    buffer_putc(b, ' ');
    buffer_putulong(b, tm->tm_year + 1900);
  }
}

/* -F / -p marker after the name */
static void
ls_put_suffix(buffer* b, unsigned int mode) {
  if(S_ISDIR(mode) && (ls_o.slash || ls_o.classify))
    buffer_putc(b, '/');
  else if(ls_o.classify) {
    if(S_ISREG(mode) && (mode & 0111))
      buffer_putc(b, '*');
#ifdef S_ISLNK
    else if(S_ISLNK(mode))
      buffer_putc(b, '@');
#endif
    else if(S_ISFIFO(mode))
      buffer_putc(b, '|');
#ifdef S_ISSOCK
    else if(S_ISSOCK(mode))
      buffer_putc(b, '=');
#endif
  }
}

/* prints one entry: [inode ][mode links owner group size date ]name[marker][ -> target].
 * The entry was lstat()ed, so a symlink shows as itself, with its target.
 * ----------------------------------------------------------------------- */
static void
ls_print(const struct ls_ent* e) {
  const struct stat* st = &e->st;

  if(ls_o.inode) {
    buffer_putulong(fd_out->w, (unsigned long)st->st_ino);
    buffer_putspace(fd_out->w);
  }

  if(ls_o.long_fmt) {
    ls_put_mode(fd_out->w, st->st_mode);
    buffer_putspace(fd_out->w);
    buffer_putulong0(fd_out->w, st->st_nlink, 3);
    buffer_putspace(fd_out->w);
    ls_put_user(fd_out->w, (unsigned long)st->st_uid);
    buffer_putspace(fd_out->w);
    ls_put_group(fd_out->w, (unsigned long)st->st_gid);
    buffer_putspace(fd_out->w);
    ls_put_size(fd_out->w, (uint64)st->st_size, 6);
    buffer_putspace(fd_out->w);
    ls_put_time(fd_out->w, st->st_mtime);
    buffer_putspace(fd_out->w);
  }

  buffer_puts(fd_out->w, e->name);
  ls_put_suffix(fd_out->w, st->st_mode);

#ifdef S_ISLNK
  if(ls_o.long_fmt && S_ISLNK(st->st_mode)) {
    char target[4096 + 1];
    ssize_t n = readlink(e->path, target, sizeof(target) - 1);

    if(n > 0) {
      buffer_puts(fd_out->w, " -> ");
      buffer_put(fd_out->w, target, n);
    }
  }
#endif

  buffer_putnlflush(fd_out->w);
}

static void
ls_free(struct ls_ent* v, unsigned int n) {
  unsigned int i;

  for(i = 0; i < n; i++) {
    alloc_free(v[i].name);
    alloc_free(v[i].path);
  }

  alloc_free(v);
}

/* prints sorted entries, preceded by "total N" (512-byte blocks) in a directory listing
 * with -l.
 * ----------------------------------------------------------------------- */
static void
ls_show(struct ls_ent* v, unsigned int n, int in_dir) {
  unsigned int i;

  if(!ls_o.unsorted)
    qsort(v, n, sizeof(*v), ls_cmp);

  if(in_dir && ls_o.long_fmt) {
    unsigned long total = 0;

    for(i = 0; i < n; i++)
      total += (unsigned long)v[i].st.st_blocks;

    buffer_puts(fd_out->w, "total ");
    buffer_putulong(fd_out->w, total);
    buffer_putnlflush(fd_out->w);
  }

  for(i = 0; i < n; i++)
    ls_print(&v[i]);
}

/* builds an entry for "dir/name" (or just "name"); 0 when lstat() fails, reported on stderr */
static int
ls_stat(char* argv[], struct ls_ent* e, const char* dir, const char* name) {
  stralloc full;

  stralloc_init(&full);

  if(dir) {
    stralloc_copys(&full, dir);

    if(full.len && full.s[full.len - 1] != '/')
      stralloc_catc(&full, '/');

    stralloc_cats(&full, name);
  } else {
    stralloc_copys(&full, name);
  }

  stralloc_nul(&full);
  e->name = str_dup(name);
  e->path = str_dup(full.s);
  stralloc_free(&full);

  if(lstat(e->path, &e->st) == -1) {
    builtin_error(argv, e->path);
    alloc_free(e->name);
    alloc_free(e->path);
    return 0;
  }

  return 1;
}

/* lists one directory; with -R then descends into each subdirectory, headed "path:" */
static int
ls_dir(char* argv[], const char* path, int header) {
  DIR* dp;
  struct dirent* de;
  struct ls_ent* v = NULL;
  unsigned int n = 0, nalloc = 0, i;
  int ret = 0;

  if(!(dp = opendir(path)))
    return builtin_error(argv, (char*)path);

  if(header) {
    buffer_puts(fd_out->w, path);
    buffer_putc(fd_out->w, ':');
    buffer_putnlflush(fd_out->w);
  }

  while((de = readdir(dp))) {
    int dot = !str_diff(de->d_name, ".") || !str_diff(de->d_name, "..");

    if(dot ? !ls_o.all : (!ls_o.all && !ls_o.almost && de->d_name[0] == '.'))
      continue;

    if(n >= nalloc) {
      nalloc = nalloc ? nalloc * 2 : 16;
      v = alloc_re(v, nalloc * sizeof(*v));
    }

    if(ls_stat(argv, &v[n], path, de->d_name))
      n++;
    else
      ret = 1;
  }

  closedir(dp);
  ls_show(v, n, 1);

  if(ls_o.recurse)
    for(i = 0; i < n; i++)
      if(S_ISDIR(v[i].st.st_mode) && str_diff(v[i].name, ".") && str_diff(v[i].name, "..")) {
        buffer_putnlflush(fd_out->w);

        if(ls_dir(argv, v[i].path, 1))
          ret = 1;
      }

  ls_free(v, n);
  return ret;
}

const char help_ls[] = "    List directory contents.\n"
                       "\n"
                       "    -a              include entries starting with '.', and '.' and '..'\n"
                       "    -A              like -a, without '.' and '..'\n"
                       "    -d              list directories themselves, not their contents\n"
                       "    -f              all entries, in directory order (-a, no sorting)\n"
                       "    -F              mark directories '/', executables '*', links '@', fifos '|'\n"
                       "    -i              print the inode number first\n"
                       "    -l              long format: mode, links, owner, group, size, date\n"
                       "    -p              mark directories with '/'\n"
                       "    -R              list subdirectories recursively\n"
                       "    -r              reverse the sort order\n"
                       "    -S              sort by size, largest first\n"
                       "    -t              sort by modification time, newest first\n"
                       "    -1              one entry per line (default)\n"
                       "    file            file or directory to list; defaults to '.'\n";

int
builtin_ls(int argc, char* argv[]) {
  int c, ret = 0, nargs, i;
  char* single_dot[] = {".", NULL};
  char** args;
  struct ls_ent *files = NULL, *dirs = NULL;
  unsigned int nfiles = 0, ndirs = 0, j;

  byte_zero(&ls_o, sizeof(ls_o));

  while((c = shell_getopt(argc, argv, "AadFfilpRrSt1")) > 0) {
    switch(c) {
      case 'A': ls_o.almost = 1; break;
      case 'a': ls_o.all = 1; break;
      case 'd': ls_o.dirs = 1; break;
      case 'F': ls_o.classify = 1; break;
      case 'f': ls_o.unsorted = ls_o.all = 1; break;
      case 'i': ls_o.inode = 1; break;
      case 'l': ls_o.long_fmt = 1; break;
      case 'p': ls_o.slash = 1; break;
      case 'R': ls_o.recurse = 1; break;
      case 'r': ls_o.reverse = 1; break;
      case 'S': ls_o.by_size = 1; break;
      case 't': ls_o.by_time = 1; break;
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

  /* POSIX: every non-directory operand first, then each directory; a symlink to a
     directory is a directory unless -l (or -d) asks about the link itself */
  files = alloc_zero(nargs * sizeof(*files));
  dirs = alloc_zero(nargs * sizeof(*dirs));

  for(i = 0; i < nargs; i++) {
    struct ls_ent e;
    struct stat st;

    if(!ls_stat(argv, &e, NULL, args[i])) {
      ret = 1;
      continue;
    }

    if(!ls_o.dirs && !(ls_o.long_fmt && S_ISLNK(e.st.st_mode)) && stat(e.path, &st) == 0 && S_ISDIR(st.st_mode))
      dirs[ndirs++] = e;
    else
      files[nfiles++] = e;
  }

  ls_show(files, nfiles, 0);

  if(!ls_o.unsorted)
    qsort(dirs, ndirs, sizeof(*dirs), ls_cmp);

  for(j = 0; j < ndirs; j++) {
    if(nfiles || j)
      buffer_putnlflush(fd_out->w);

    if(ls_dir(argv, dirs[j].path, nargs > 1 || ls_o.recurse))
      ret = 1;
  }

  ls_free(files, nfiles);
  ls_free(dirs, ndirs);
  return ret;
}
#endif /* BUILTIN_LS */
