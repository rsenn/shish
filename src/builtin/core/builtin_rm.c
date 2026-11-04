#include "builtin_config.h"

#if BUILTIN_MV || BUILTIN_RM

#include "../../builtin.h"
#include "../../fdtable.h"
#include "../../../lib/shell.h"
#include "../../../lib/stralloc.h"
#include "../../../lib/str.h"
#include "../../../lib/unix.h"
#include "../../../lib/walk.h"
#include <unistd.h>
#include <errno.h>
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

/* removes a single path, and everything below it if it is a directory.
 * The walk is physical: a symlink is unlinked, never entered, so
 * "rm -r symlink-to-dir" removes just the symlink. Returns 0 on success,
 * 1 on failure (already reported via builtin_error()); with 'force' set,
 * a missing path is not an error. A directory with anything left in it
 * (declined with -i, or failed) is not attempted.
 * ----------------------------------------------------------------------- */
static int
rm_tree(char* argv[], const char* root, int force, int verbose, int interactive) {
  struct walk w;
  struct walk_ent* e;
  int ret = 0;

  if(walk_open(&w, root, WALK_PRE | WALK_POST | WALK_ERR)) {
    builtin_error(argv, (char*)root);
    return 1;
  }

  while((e = walk_read(&w))) {
    int dir = e->phase == WALK_POST;

    if(e->phase == WALK_ERR) {
      if(!(force && e->err == ENOENT)) {
        errno = e->err;
        builtin_error(argv, (char*)e->path);
        ret = 1;
        walk_keep(&w);
      }
      continue;
    }

    if(e->phase == WALK_PRE) {
      if(interactive && !rm_confirm("descend into directory", e->path)) {
        walk_skip(&w);
        walk_keep(&w);
      }
      continue;
    }

    /* a kept directory (declined or failed below it) stays, without asking */
    if(dir ? e->kept || (interactive && !rm_confirm("remove directory", e->path)) : (interactive && !rm_confirm("remove", e->path))) {
      walk_keep(&w);
      continue;
    }

    if((dir ? rmdir(e->path) : unlink(e->path)) == -1) {
      if(!(force && errno == ENOENT)) {
        builtin_error(argv, (char*)e->path);
        ret = 1;
      }
      walk_keep(&w);
      continue;
    }

    if(verbose) {
      buffer_putm_internal(fd_out->w, dir ? "removed directory '" : "removed '", e->path, "'", 0);
      buffer_putnlflush(fd_out->w);
    }
  }

  walk_close(&w);
  return ret;
}

int
builtin_rm_tree(char* argv[], stralloc* path, int force, int verbose) {
  return rm_tree(argv, path->s, force, verbose, 0);
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
                       "    file            file (or, with -r, directory) to remove\n"
                       "\n"
                       "    long options: --force -f, --interactive -i, --recursive -r,\n"
                       "                  --recursive -R, --dir -d, --verbose -v\n";

int
builtin_rm(int argc, char* argv[]) {
  int c, ret;
  int verbose = 0, force = 0, recursive = 0, interactive = 0, dirs = 0;
  char* p;

  /* check options */
  while((c = shell_getopt_long(argc, argv, "vfrRid", "force=f interactive=i recursive=r recursive=R dir=d verbose=v")) > 0) {
    switch(c) {
      case 'v': verbose = 1; break;
      case 'f': force = 1, interactive = 0; break;
      case 'i': interactive = 1, force = 0; break;
      case 'd': dirs = 1; break;
      case 'r':
      case 'R': recursive = 1; break;
      default: return builtin_invopt(argv);
    }
  }

  if(!argv[shell_optind]) {
    if(force)
      return 0;

    builtin_errmsg(argv, "missing operand", NULL);
    return 1;
  }

  if(recursive) {
    int failed = 0;

    while((p = argv[shell_optind++])) {
      if(rm_refuse(argv, p) || rm_tree(argv, p, force, verbose, interactive)) {
        failed = 1;

        if(!force)
          return 1;
      }
    }

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
