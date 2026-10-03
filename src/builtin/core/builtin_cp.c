#include "../../builtin.h"
#include "../../fdtable.h"
#include "../../sh.h"
#include "../../../lib/shell.h"
#include "../../../lib/stralloc.h"
#include "../../../lib/alloc.h"
#include "../../../lib/byte.h"
#include "../../../lib/str.h"
#include "../../../lib/fmt.h"
#include "../../../lib/windoze.h"
#include "../../../lib/unix.h"
#include "config.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h> /* rename() only */
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <utime.h>

#ifndef HAVE_LSTAT
#define lstat stat
#endif

/* no ownership, fifos or symlinks to copy on these targets */
#if WINDOWS_NATIVE || defined(__wasi__) || defined(__wasm__)
#define CPMV_NOUNIX 1
#endif

#define CPMV_BUFSIZE 65536

/* options and per-run state shared by cp and mv
 * ----------------------------------------------------------------------- */
struct cpmv {
  char** argv;
  int mv;
  int recurse;     /* cp -R */
  int follow;      /* cp: 0 = -P, 1 = -H, 2 = -L; -1 = not given */
  int force;       /* -f */
  int interactive; /* -i */
  int noclobber;   /* -n */
  int preserve;    /* -p */
  int verbose;     /* -v */
  mode_t mask;     /* the umask, read once */
  char* buf;       /* CPMV_BUFSIZE bytes, allocated on first use */
};

const char help_cp[] = "    Copy files.\n"
                       "\n"
                       "    -R, -r          copy directories recursively\n"
                       "    -H, -L, -P      with -R: follow command-line symlinks, all symlinks, none (default)\n"
                       "    -f              remove an unwritable destination and try again\n"
                       "    -i              ask before overwriting\n"
                       "    -n              never overwrite an existing file\n"
                       "    -p              keep times, owner and mode\n"
                       "    -a              -R -P -p\n"
                       "    -v              print each file copied\n"
                       "    source          file (or, with -R, directory) to copy\n"
                       "    target          file, or directory to copy into\n";

const char help_mv[] = "    Move or rename files.\n"
                       "\n"
                       "    -f              do not ask before overwriting\n"
                       "    -i              ask before overwriting\n"
                       "    -n              never overwrite an existing file\n"
                       "    -v              print each file moved\n"
                       "    source          file or directory to move\n"
                       "    target          new name, or directory to move into\n";

/* "name" part of a path without its trailing slashes: "a/b//" -> "b"
 *
 *   const char*  s    the path
 *   size_t*      len  receives the length of the name
 * ----------------------------------------------------------------------- */
static const char*
cpmv_base(const char* s, size_t* len) {
  size_t n = str_len(s), i;

  while(n > 1 && s[n - 1] == '/')
    n--;

  for(i = n; i > 0 && s[i - 1] != '/'; i--) {}

  *len = n - i;
  return s + i;
}

/* the same file: equal (device, inode)
 * ----------------------------------------------------------------------- */
static int
cpmv_same(const struct stat* a, const struct stat* b) {
  return a->st_dev == b->st_dev && a->st_ino == b->st_ino;
}

/* is "dst" the directory "dir" or somewhere below it?
 *
 *   dst/..  /..  walks up until the parent is the directory itself (the root)
 * ----------------------------------------------------------------------- */
static int
cpmv_inside(const struct stat* dir, const char* dst) {
  stralloc p;
  struct stat st, up;
  size_t n = str_len(dst);
  int ret = 0;

  stralloc_init(&p);
  stralloc_copys(&p, dst);
  stralloc_nul(&p);

  /* a destination that does not exist yet starts at its parent */
  if(stat(p.s, &st) == -1) {
    while(n > 0 && p.s[n - 1] != '/')
      n--;

    p.len = n ? n : 0;
    stralloc_cats(&p, n ? "" : ".");
    stralloc_nul(&p);
  }

  for(;;) {
    if(stat(p.s, &st) == -1)
      break;

    if(cpmv_same(&st, dir)) {
      ret = 1;
      break;
    }

    stralloc_cats(&p, "/..");
    stralloc_nul(&p);

    if(stat(p.s, &up) == -1 || cpmv_same(&st, &up))
      break;
  }

  stralloc_free(&p);
  return ret;
}

/* ask "cmd: overwrite 'dst'? " and read one line from standard input
 *
 * returns 1 for a reply starting with y or Y; end of input is "no"
 * ----------------------------------------------------------------------- */
static int
cpmv_confirm(struct cpmv* o, const char* dst) {
  char c, first = 0;
  int n = 0;

  buffer_putm_internal(fd_err->w, o->argv[0], ": overwrite '", dst, "'? ", 0);
  buffer_flush(fd_err->w);

  while(buffer_getc(fdtable[0]->r, &c) > 0 && c != '\n')
    if(n++ == 0)
      first = c;

  return first == 'y' || first == 'Y';
}

static void
cpmv_say(struct cpmv* o, const char* src, const char* dst) {
  if(o->verbose) {
    buffer_putm_internal(fd_out->w, "'", src, "' -> '", dst, "'", 0);
    buffer_putnlflush(fd_out->w);
  }
}

/* give the copy the characteristics of the original: with -p the times,
 * owner and mode (set-id bits only survive a successful chown), else the
 * mode of the original under the umask
 * ----------------------------------------------------------------------- */
static void
cpmv_attrs(struct cpmv* o, const char* dst, const struct stat* st, int link) {
  mode_t mode = st->st_mode & 07777;

  if(link)
    return;

  if(o->preserve) {
#ifndef CPMV_NOUNIX
    if(chown(dst, st->st_uid, st->st_gid) == -1)
      mode &= ~(mode_t)(S_ISUID | S_ISGID);
#endif
    {
      struct utimbuf ut;

      ut.actime = st->st_atime;
      ut.modtime = st->st_mtime;
      utime(dst, &ut);
    }
  } else {
    mode &= ~o->mask & 0777;
  }

  chmod(dst, mode);
}

/* copy the bytes of one regular file
 * returns 0, or 1 after reporting the error
 * ----------------------------------------------------------------------- */
static int
cpmv_data(struct cpmv* o, const char* src, const char* dst, const struct stat* st) {
  int in, out, ret = 0, flags = O_WRONLY | O_CREAT | O_TRUNC;
  struct stat dst_st;
  ssize_t n;

  if((in = open(src, O_RDONLY)) == -1) {
    builtin_error(o->argv, (char*)src);
    return 1;
  }

  /* an existing destination keeps its inode and mode; a new one gets the
     mode of the original (the kernel applies the umask) */
  if((out = open(dst, flags, st->st_mode & 0777)) == -1 && o->force && lstat(dst, &dst_st) == 0) {
    unlink(dst);
    out = open(dst, O_WRONLY | O_CREAT | O_EXCL, st->st_mode & 0777);
  }

  if(out == -1) {
    builtin_error(o->argv, (char*)dst);
    close(in);
    return 1;
  }

  if(!o->buf)
    o->buf = alloc(CPMV_BUFSIZE);

  while((n = read(in, o->buf, CPMV_BUFSIZE)) != 0) {
    ssize_t off = 0;

    if(n < 0) {
      if(errno == EINTR)
        continue;

      builtin_error(o->argv, (char*)src);
      ret = 1;
      break;
    }

    while(off < n) {
      ssize_t w = write(out, o->buf + off, (size_t)(n - off));

      if(w < 0) {
        if(errno == EINTR)
          continue;

        builtin_error(o->argv, (char*)dst);
        ret = 1;
        goto done;
      }

      off += w;
    }
  }

done:
  close(in);

  if(close(out) == -1 && !ret) {
    builtin_error(o->argv, (char*)dst);
    ret = 1;
  }

  return ret;
}

static int cpmv_one(struct cpmv* o, const char* src, const char* dst, int top);

/* copy a directory hierarchy: the directory is created writable while its
 * contents go in, and gets its real mode afterwards
 * ----------------------------------------------------------------------- */
static int
cpmv_dir(struct cpmv* o, const char* src, const char* dst, const struct stat* st) {
  DIR* dp;
  struct dirent* de;
  stralloc s, d;
  struct stat ex;
  size_t slen, dlen;
  int ret = 0;

  if(lstat(dst, &ex) == 0) {
    if(!S_ISDIR(ex.st_mode)) {
      builtin_errmsg(o->argv, (char*)dst, "not a directory");
      return 1;
    }
  } else if(mkdir(dst, (st->st_mode | S_IRWXU) & 07777) == -1) {
    builtin_error(o->argv, (char*)dst);
    return 1;
  }

  if(!(dp = opendir(src))) {
    builtin_error(o->argv, (char*)src);
    return 1;
  }

  stralloc_init(&s);
  stralloc_init(&d);
  stralloc_copys(&s, src);
  stralloc_catc(&s, '/');
  stralloc_copys(&d, dst);
  stralloc_catc(&d, '/');
  slen = s.len;
  dlen = d.len;

  while((de = readdir(dp))) {
    if(!str_diff(de->d_name, ".") || !str_diff(de->d_name, ".."))
      continue;

    s.len = slen;
    d.len = dlen;
    stralloc_cats(&s, de->d_name);
    stralloc_cats(&d, de->d_name);
    stralloc_nul(&s);
    stralloc_nul(&d);

    if(cpmv_one(o, s.s, d.s, 0))
      ret = 1;
  }

  closedir(dp);
  stralloc_free(&s);
  stralloc_free(&d);

  cpmv_attrs(o, dst, st, 0);
  return ret;
}

/* copy one source to one destination name
 *
 *   int  top  1 for a command-line operand (what -H follows)
 * returns 0, or 1 after reporting
 * ----------------------------------------------------------------------- */
static int
cpmv_one(struct cpmv* o, const char* src, const char* dst, int top) {
  struct stat st, dst_st;
  int follow = o->follow < 0 ? (o->recurse ? 0 : 2) : o->follow;
  int link, have_dst;

  /* -L follows every symlink, -H the operands, -P none */
  if((follow == 2 || (follow == 1 && top) ? stat(src, &st) : lstat(src, &st)) == -1) {
    builtin_error(o->argv, (char*)src);
    return 1;
  }

  link = S_ISLNK(st.st_mode);
  have_dst = lstat(dst, &dst_st) == 0;

  if(have_dst && cpmv_same(&st, &dst_st) && !link) {
    stralloc msg;

    stralloc_init(&msg);
    stralloc_cats(&msg, "'");
    stralloc_cats(&msg, src);
    stralloc_cats(&msg, "' and '");
    stralloc_cats(&msg, dst);
    stralloc_cats(&msg, "' are the same file");
    stralloc_nul(&msg);
    builtin_errmsg(o->argv, msg.s, 0);
    stralloc_free(&msg);
    return 1;
  }

  if(S_ISDIR(st.st_mode)) {
    if(!o->recurse) {
      stralloc msg;

      stralloc_init(&msg);
      stralloc_cats(&msg, "-r not specified; omitting directory '");
      stralloc_cats(&msg, src);
      stralloc_cats(&msg, "'");
      stralloc_nul(&msg);
      builtin_errmsg(o->argv, msg.s, 0);
      stralloc_free(&msg);
      return 1;
    }

    if(cpmv_inside(&st, dst)) {
      builtin_errmsg(o->argv, (char*)dst, "cannot copy a directory into itself");
      return 1;
    }

    cpmv_say(o, src, dst);
    return cpmv_dir(o, src, dst, &st);
  }

  if(have_dst) {
    if(S_ISDIR(dst_st.st_mode)) {
      builtin_errmsg(o->argv, (char*)dst, "cannot overwrite a directory with a non-directory");
      return 1;
    }

    if(o->noclobber)
      return 0;

    if(o->interactive && !cpmv_confirm(o, dst))
      return 0;
  }

  if(link) {
#ifndef CPMV_NOUNIX
    char target[4096];
    ssize_t n = readlink(src, target, sizeof(target) - 1);

    if(n == -1) {
      builtin_error(o->argv, (char*)src);
      return 1;
    }

    target[n] = '\0';

    if(have_dst)
      unlink(dst);

    if(symlink(target, dst) == -1) {
      builtin_error(o->argv, (char*)dst);
      return 1;
    }

    cpmv_say(o, src, dst);
    return 0;
#else
    builtin_errmsg(o->argv, (char*)src, "cannot copy a symbolic link");
    return 1;
#endif
  }

  if(S_ISREG(st.st_mode)) {
    if(cpmv_data(o, src, dst, &st))
      return 1;

    cpmv_attrs(o, dst, &st, 0);
    cpmv_say(o, src, dst);
    return 0;
  }

#ifndef CPMV_NOUNIX
  if(S_ISFIFO(st.st_mode) && o->recurse) {
    if(have_dst)
      unlink(dst);

    if(mkfifo(dst, st.st_mode & 0777) == -1) {
      builtin_error(o->argv, (char*)dst);
      return 1;
    }

    cpmv_attrs(o, dst, &st, 0);
    cpmv_say(o, src, dst);
    return 0;
  }
#endif

  builtin_errmsg(o->argv, (char*)src, "cannot copy a special file");
  return 1;
}

/* rename one source; across file systems copy it, then remove it
 * returns 0, or 1 after reporting
 * ----------------------------------------------------------------------- */
static int
mv_one(struct cpmv* o, const char* src, const char* dst) {
  struct stat st, dst_st;
  int have_dst;

  if(lstat(src, &st) == -1) {
    builtin_error(o->argv, (char*)src);
    return 1;
  }

  if((have_dst = lstat(dst, &dst_st) == 0)) {
    if(cpmv_same(&st, &dst_st)) {
      stralloc msg;

      stralloc_init(&msg);
      stralloc_cats(&msg, "'");
      stralloc_cats(&msg, src);
      stralloc_cats(&msg, "' and '");
      stralloc_cats(&msg, dst);
      stralloc_cats(&msg, "' are the same file");
      stralloc_nul(&msg);
      builtin_errmsg(o->argv, msg.s, 0);
      stralloc_free(&msg);
      return 1;
    }

    if(o->noclobber)
      return 0;

    /* the prompt: -i, or an unwritable destination while reading from a terminal */
    if(!o->force && (o->interactive || (access(dst, W_OK) == -1 && isatty(0))) && !cpmv_confirm(o, dst))
      return 0;
  }

  if(S_ISDIR(st.st_mode) && cpmv_inside(&st, dst)) {
    builtin_errmsg(o->argv, (char*)dst, "cannot move a directory into itself");
    return 1;
  }

  if(rename(src, dst) == 0) {
    cpmv_say(o, src, dst);
    return 0;
  }

  if(errno != EXDEV) {
    builtin_error(o->argv, (char*)dst);
    return 1;
  }

  /* another file system: copy next to the destination, rename the copy into
     place, and only then remove the source */
  {
    stralloc tmp, srcpath;
    int ret = 0, recurse = o->recurse, follow = o->follow, preserve = o->preserve;
    int force = o->force, interactive = o->interactive, noclobber = o->noclobber, verbose = o->verbose;
    char num[24];

    stralloc_init(&tmp);
    stralloc_init(&srcpath);
    stralloc_copys(&tmp, dst);
    stralloc_cats(&tmp, ".mv.");
    num[fmt_ulong(num, (unsigned long)getpid())] = '\0';
    stralloc_cats(&tmp, num);
    stralloc_nul(&tmp);

    o->recurse = 1;
    o->follow = 0;
    o->preserve = 1;
    o->force = o->interactive = o->noclobber = o->verbose = 0;

    ret = cpmv_one(o, src, tmp.s, 1);

    o->recurse = recurse;
    o->follow = follow;
    o->preserve = preserve;
    o->force = force;
    o->interactive = interactive;
    o->noclobber = noclobber;
    o->verbose = verbose;

    if(!ret && rename(tmp.s, dst) == -1) {
      builtin_error(o->argv, (char*)dst);
      ret = 1;
    }

    if(ret) {
      /* leave the source alone, drop the partial copy */
      stralloc_copy(&srcpath, &tmp);
      stralloc_nul(&srcpath);
      builtin_rm_tree(o->argv, &srcpath, 1, 0);
    } else {
      stralloc_copys(&srcpath, src);
      stralloc_nul(&srcpath);
      ret = builtin_rm_tree(o->argv, &srcpath, 0, 0);
      cpmv_say(o, src, dst);
    }

    stralloc_free(&tmp);
    stralloc_free(&srcpath);
    return ret;
  }
}

int
builtin_cpmv(int argc, char* argv[]) {
  struct cpmv o;
  const char* optstr;
  struct stat tst;
  char *target, *src;
  int c, nsrc, is_dir, ret = 0;
  stralloc dst;

  byte_zero(&o, sizeof(o));
  o.argv = argv;
  o.follow = -1;

  if(!str_diff(argv[0], "cp"))
    optstr = "RrHLPfipnva";
  else if(!str_diff(argv[0], "mv")) {
    o.mv = 1;
    optstr = "finv";
  } else {
    builtin_errmsg(argv, argv[0], "unknown name");
    return 1;
  }

  while((c = shell_getopt(argc, argv, optstr)) > 0) {
    switch(c) {
      case 'R':
      case 'r': o.recurse = 1; break;
      case 'H': o.follow = 1; break;
      case 'L': o.follow = 2; break;
      case 'P': o.follow = 0; break;
      case 'f': o.force = 1; o.interactive = 0; break;
      case 'i': o.interactive = 1; o.force = o.mv ? 0 : o.force; break;
      case 'n': o.noclobber = 1; break;
      case 'p': o.preserve = 1; break;
      case 'v': o.verbose = 1; break;
      case 'a':
        o.recurse = 1;
        o.follow = 0;
        o.preserve = 1;
        break;
      default: builtin_invopt(argv); return 1;
    }
  }

  nsrc = argc - shell_optind - 1;

  if(nsrc < 1) {
    builtin_errmsg(argv, nsrc < 0 ? "missing file operand" : argv[shell_optind], nsrc < 0 ? 0 : "missing destination file operand");
    return 1;
  }

  target = argv[argc - 1];
  is_dir = stat(target, &tst) == 0 && S_ISDIR(tst.st_mode);

  if(nsrc > 1 && !is_dir) {
    builtin_errmsg(argv, target, "not a directory");
    return 1;
  }

  /* the umask can only be read by setting it */
  o.mask = umask(0);
  umask(o.mask);

  stralloc_init(&dst);

  while(shell_optind < argc - 1) {
    size_t blen;
    const char* base;

    src = argv[shell_optind++];

    if(is_dir) {
      base = cpmv_base(src, &blen);
      stralloc_copys(&dst, target);

      if(dst.len && dst.s[dst.len - 1] != '/')
        stralloc_catc(&dst, '/');

      stralloc_catb(&dst, base, blen);
    } else {
      stralloc_copys(&dst, target);
    }

    stralloc_nul(&dst);

    if(o.mv ? mv_one(&o, src, dst.s) : cpmv_one(&o, src, dst.s, 1))
      ret = 1;
  }

  stralloc_free(&dst);

  if(o.buf)
    alloc_free(o.buf);

  return ret;
}
