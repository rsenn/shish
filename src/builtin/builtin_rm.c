#include "../builtin.h"
#include "../fdtable.h"
#include "../../lib/shell.h"
#include "../../lib/stralloc.h"
#include "../../lib/str.h"
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

/* removes a single path, recursing into it first if it's a directory
 * and not itself a symlink (lstat(), not stat(), so "rm -r
 * symlink-to-dir" removes just the symlink). Returns 0 on success, 1
 * on failure (already reported via builtin_error()); with 'force'
 * set, a missing path is not an error.
 * ----------------------------------------------------------------------- */
static int
rm_recursive(char* argv[], stralloc* path, int force, int verbose) {
  struct stat st;
  int ret = 0;

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

      if(rm_recursive(argv, path, force, verbose))
        ret = 1;
    }

    closedir(dp);
    path->len = dirlen;
    stralloc_nul(path);

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

/* output stuff
 * ----------------------------------------------------------------------- */
const char help_rm[] = "    Remove files or directories.\n"
                       "\n"
                       "    -r, -R          remove directories and their contents recursively\n"
                       "    -f              ignore missing files, never prompt\n"
                       "    -v              print each file/directory removed\n"
                       "    file            file (or, with -r, directory) to remove\n";

int
builtin_rm(int argc, char* argv[]) {
  int c, ret;
  int verbose = 0, force = 0, recursive = 0;
  char* p;

  /* check options */
  while((c = shell_getopt(argc, argv, "vfrR")) > 0) {
    switch(c) {
      case 'v': verbose = 1; break;
      case 'f': force = 1; break;
      case 'r':
      case 'R': recursive = 1; break;
      default: builtin_invopt(argv); return 1;
    }
  }

  if(recursive) {
    stralloc path;
    int failed = 0;

    stralloc_init(&path);

    while((p = argv[shell_optind++])) {
      stralloc_copys(&path, p);
      stralloc_nul(&path);

      if(rm_recursive(argv, &path, force, verbose)) {
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
    ret = unlink(p);

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
