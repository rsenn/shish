#include "../builtin.h"
#include "../../lib/byte.h"
#include "../fdtable.h"
#include "../sh.h"
#include "../../lib/path.h"
#include "../../lib/str.h"
#include "../var.h"
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <sys/stat.h>
#include "../../lib/windoze.h"

#if WINDOWS_NATIVE
#include <windows.h>
#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif
#else
#include <unistd.h>
#endif

/* change working directory
 * ----------------------------------------------------------------------- */
const char help_cd[] = "    Change the shell's current working directory.\n"
                       "\n"
                       "    -L              keep symlinks in the resulting path (default)\n"
                       "    -P              resolve symlinks away from the resulting path\n"
                       "    directory       directory to change into; default is $HOME\n"
                       "    -               change to $OLDPWD and print the new directory\n"
                       "\n"
                       "    A relative directory is first tried against each prefix in\n"
                       "    $CDPATH; $PWD and $OLDPWD are updated on success.\n";

int
builtin_cd(int argc, char* argv[]) {
  int c, ok = 0;
  int symbolic = 1;
  int print_path = 0;
  const char* arg;
  size_t len, n;
  stralloc newcwd;

  /* check options, -L for symlink, -P for physical path */
  while((c = shell_getopt(argc, argv, "LP")) > 0) {
    switch(c) {
      case 'L': symbolic = 1; break;
      case 'P': symbolic = 0; break;
      default: builtin_invopt(argv); return 1;
    }
  }

  arg = argv[shell_optind];
  stralloc_init(&newcwd);

  /* "cd -" means $OLDPWD, and the resulting directory is echoed,
     same as a CDPATH hit -- POSIX 2.9.1 (Special Built-In Utilities),
     "cd" description */
  if(arg != NULL && !str_diff(arg, "-")) {
    arg = var_value("OLDPWD", &len);

    if(arg[0] == '\0') {
      sh_msg("OLDPWD variable not set!");
      return 1;
    }

    print_path = 1;
  }

  /* empty argument means chdir(HOME) */
  if(arg == NULL) {
    arg = var_value("HOME", &len);

    if(arg[0] == '\0') {
      sh_msg("HOME variable not set!");
      return 1;
    }
  }

  len = str_len(arg);

  /* POSIX: CDPATH is not used if the operand begins with '/', or with
     a "." or ".." path component (cd-cdpath-applies-to-dot-operand) */
  if(arg[0] != '/' && !(arg[0] == '.' &&
                        (arg[1] == '/' || arg[1] == '\0' ||
                         (arg[1] == '.' && (arg[2] == '/' || arg[2] == '\0'))))) {
    char path[PATH_MAX + 1];
    const char* cdpath;

    /* loop through colon-separated CDPATH variable */
    cdpath = var_value("CDPATH", NULL);

    do {
      /* too much, too much :) */
      if((n = str_chr(cdpath, ':')) + len + 1 > PATH_MAX) {
        /* set error code and print the longer string in the error msg */
        errno = ENAMETOOLONG;
        return builtin_errmsgn(argv,
                               (n > len ? cdpath : arg),
                               (n > len ? n : len),
                               strerror(errno));
      }

      /* copy path prefix from cdpath if present */
      if(n) {
        byte_copy(path, n, cdpath);
        cdpath += n;
        path[n++] = '/';
      }

      /* copy the argument and canonicalize */
      str_copy(&path[n], arg);

      /* path_canonicalize() only follows symlinks to resolve them
         (in -P mode); it never verifies that each component actually
         exists and is a directory, so a fully-formed "ok" result
         alone can't tell a real CDPATH hit from a nonexistent one, or
         reject a candidate that runs through a non-directory
         component (e.g. "file/../dev") -- confirm against the real
         filesystem with stat() on the *raw*, uncanonicalized
         candidate first, so an invalid intermediate component is
         caught the same way a real chdir() would catch it
         (cd-cdpath-only-tries-first-component,
         cd-accepts-non-directory-path-component) */
      {
        struct stat st;

        if(stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
          /* newcwd must be cleared before each attempt --
             path_realpath() only prepends the shell's cwd when
             sa->len is still zero, so a previous failed candidate's
             leftover contents would otherwise suppress that prepend
             on the next component and corrupt it */
          stralloc_zero(&newcwd);
          ok = path_realpath(path, &newcwd, symbolic, &sh->cwd);
        } else {
          ok = 0;
        }
      }

      /* skip the colon */
      if(*cdpath == ':')
        cdpath++;
    } while(*cdpath && !ok);

    /* POSIX: if no CDPATH component (including a leading/trailing/
       double colon, i.e. the empty "current directory" component)
       yields a match, the operand is used as-is, relative to the
       current directory, exactly as if CDPATH were unset
       (cd-cdpath-exhausted-falls-back-to-cwd) */
    if(!ok) {
      struct stat st;

      n = 0;

      if(stat(arg, &st) == 0 && S_ISDIR(st.st_mode)) {
        stralloc_zero(&newcwd);
        ok = path_realpath(arg, &newcwd, symbolic, &sh->cwd);
      }
    }
  }

  /* absolute path, or an operand beginning with a "." or ".."
     component: CDPATH is not consulted */
  else {
    struct stat st;

    /* last cdpath length set to 0, because we're not using cdpath here */
    n = 0;

    /* path_realpath(), not path_canonicalize() directly -- a "."/".."
       operand is relative and must still be resolved against the
       shell's tracked cwd (path_realpath() prepends it for us, and is
       a no-op for a genuinely absolute arg) */
    if(stat(arg, &st) == 0 && S_ISDIR(st.st_mode))
      ok = path_realpath(arg, &newcwd, symbolic, &sh->cwd);
  }

  stralloc_nul(&newcwd);

  /* try to chdir() if everything's ok */
  if(ok && chdir(newcwd.s) == 0) {
    /* print path if prefix was taken from cdpath, or this was "cd -" */
    if(n || print_path) {
      buffer_putsa(fd_out->w, &newcwd);
      buffer_putnlflush(fd_out->w);
    }

    /* POSIX: OLDPWD shall be set to the value of the current working
       directory before it is changed */
    var_setv("OLDPWD", sh->cwd.s, sh->cwd.len, V_DEFAULT);

    /* set the path */
    stralloc_move(&sh->cwd, &newcwd);

    /* if the path has symlinks then set sh->cwdsym */
    sh->cwdsym = (ok > 1);

    var_setv("PWD", sh->cwd.s, sh->cwd.len, V_DEFAULT);

    return 0;
  }

  /* we failed */
  builtin_error(argv, newcwd.s);
  stralloc_free(&newcwd);
  return 1;
}
