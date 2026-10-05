#include "builtin_config.h"

#if BUILTIN_MV || BUILTIN_RM

#include "../../builtin.h"
#include "../../fdtable.h"
#include "../../../lib/shell.h"
#include "../../../lib/stralloc.h"
#include "../../../lib/str.h"
#include "../../../lib/unix.h"
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

/* -i: ask "rm: WHAT 'PATH'? " on stderr, answer from stdin; only "y"/"Y..." says yes */
static int
rm_confirm(const char* what, const char* path) {
  char c = 0, first = 0;

  buffer_putm_internal(fd_err->w, "rm: ", what, " '", path, "'? ", 0);
  buffer_flush(fd_err->w);

  while(buffer_getc(fd_in->r, &c) > 0 && c != '\n')
    if(!first)
      first = c;

  return first == 'y' || first == 'Y';
}

/* removes a single path, recursing into it first if it's a directory
 * and not itself a symlink (lstat(), not stat(), so "rm -r
 * symlink-to-dir" removes just the symlink). Returns 0 on success, 1
 * on failure (already reported via builtin_error()); with 'force'
 * set, a missing path is not an error.
 * ----------------------------------------------------------------------- */
static int
rm_tree(char* argv[], stralloc* path, int force, int verbose, int interactive, int* kept) {
  struct stat st;
  int ret = 0, skipped = 0;

  if(lstat(path->s, &st) == -1) {
    if(force && errno == ENOENT)
      return 0;

    builtin_error(argv, path->s);
    return 1;
  }

  if(S_ISDIR(st.st_mode)) {
    DIR* dp;
    struct dirent* de;
    size_t dirlen = path->len;

    if(interactive && !rm_confirm("descend into directory", path->s)) {
      *kept = 1;
      return 0;
    }

    if(!(dp = opendir(path->s))) {
      builtin_error(argv, path->s);
      return 1;
    }

    while((de = readdir(dp))) {
      if(!str_diff(de->d_name, ".") || !str_diff(de->d_name, ".."))
        continue;

      path->len = dirlen;
      stralloc_catc(path, '/');
      stralloc_cats(path, de->d_name);
      stralloc_nul(path);

      if(rm_tree(argv, path, force, verbose, interactive, &skipped))
        ret = 1;
    }

    closedir(dp);
    path->len = dirlen;
    stralloc_nul(path);

    /* a declined entry keeps its directory; so does declining the directory itself */
    if(skipped || (interactive && !rm_confirm("remove directory", path->s))) {
      *kept = 1;
      return ret;
    }

    if(rmdir(path->s) == -1) {
      if(!(force && errno == ENOENT)) {
        builtin_error(argv, path->s);
        ret = 1;
      }
    } else if(verbose) {
      buffer_putm_internal(fd_out->w, "removed directory '", path->s, "'", 0);
      buffer_putnlflush(fd_out->w);
    }

    return ret;
  }

  if(interactive && !rm_confirm("remove", path->s)) {
    *kept = 1;
    return 0;
  }

  if(unlink(path->s) == -1) {
    if(force && errno == ENOENT)
      return 0;

    builtin_error(argv, path->s);
    return 1;
  }

  if(verbose) {
    buffer_putm_internal(fd_out->w, "removed '", path->s, "'", 0);
    buffer_putnlflush(fd_out->w);
  }

  return 0;
}

int
builtin_rm_tree(char* argv[], stralloc* path, int force, int verbose) {
  int kept = 0;

  return rm_tree(argv, path, force, verbose, 0, &kept);
}

/* refuse operands that would take the working directory or the root with
 * them:
 *
 *   rm -r .  ..  a/.  a/..  /  //  /..  (symlink to / is fine: lstat)
 *
 * Returns 1 after reporting, 0 if the operand may be removed.
 * ----------------------------------------------------------------------- */
static int
rm_refuse(char* argv[], char* p) {
  struct stat st, root;
  size_t n = str_len(p);

  while(n > 1 && p[n - 1] == '/')
    n--;

  if((n == 1 && p[0] == '.') || (n >= 2 && p[n - 1] == '.' && (p[n - 2] == '/' || (p[n - 2] == '.' && (n == 2 || p[n - 3] == '/'))))) {
    builtin_errmsg(argv, p, "refusing to remove '.' or '..'");
    return 1;
  }

  if(lstat(p, &st) == 0 && stat("/", &root) == 0 && st.st_dev == root.st_dev && st.st_ino == root.st_ino) {
    builtin_errmsg(argv, p, "refusing to remove '/'");
    return 1;
  }

  return 0;
}

/* output stuff
 * ----------------------------------------------------------------------- */
const char help_rm[] = "    Remove files or directories.\n"
                       "\n"
                       "    -r, -R          remove directories and their contents recursively\n"
                       "    -f              ignore missing files, never prompt\n"
                       "    -i              ask before removing each file and directory\n"
                       "    -d              also remove empty directories\n"
                       "    -v              print each file/directory removed\n"
                       "    file            file (or, with -r, directory) to remove\n";

int
builtin_rm(int argc, char* argv[]) {
  int c, ret;
  int verbose = 0, force = 0, recursive = 0, interactive = 0, dirs = 0;
  char* p;

  /* check options */
  while((c = shell_getopt(argc, argv, "vfrRid")) > 0) {
    switch(c) {
      case 'v': verbose = 1; break;
      case 'f': force = 1, interactive = 0; break;
      case 'i': interactive = 1, force = 0; break;
      case 'd': dirs = 1; break;
      case 'r':
      case 'R': recursive = 1; break;
      default: builtin_invopt(argv); return 1;
    }
  }

  if(!argv[shell_optind]) {
    if(force)
      return 0;

    builtin_errmsg(argv, "missing operand", NULL);
    return 1;
  }

  if(recursive) {
    stralloc path;
    int failed = 0, kept = 0;

    stralloc_init(&path);

    while((p = argv[shell_optind++])) {
      stralloc_copys(&path, p);
      stralloc_nul(&path);

      if(rm_refuse(argv, p) || rm_tree(argv, &path, force, verbose, interactive, &kept)) {
        failed = 1;

        if(!force) {
          stralloc_free(&path);
          return 1;
        }
      }
    }

    stralloc_free(&path);
    return failed;
  }

  while((p = argv[shell_optind++])) {
    if(interactive && !rm_confirm("remove", p))
      continue;

    ret = unlink(p);

    /* -d: an empty directory goes too */
    if(ret == -1 && dirs && (errno == EISDIR || errno == EPERM))
      ret = rmdir(p);

    if(ret == -1) {
      if(!force) {
        builtin_error(argv, p);
        return 1;
      }

      continue;
    }

    if(verbose) {
      buffer_putm_internal(fd_out->w, "removed '", p, "'", 0);
      buffer_putnlflush(fd_out->w);
    }
  }

  return 0;
}
#endif /* BUILTIN_MV || BUILTIN_RM */
