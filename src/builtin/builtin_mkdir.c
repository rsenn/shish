#include "../builtin.h"
#include "../fdtable.h"
#include "../../lib/shell.h"
#include "../../lib/path.h"
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

/* -p: create 'dir' one path component at a time, leftmost first, so
 * every ancestor exists by the time a deeper mkdir() needs it.
 * path_len_s() (lib/path/) gives the length of the *next* component
 * starting at 'p' -- zero if 'p' is already sitting on a separator,
 * which is exactly the case right after an absolute path's leading
 * "/" (or a run of repeated "/"s anywhere in the operand): skipping
 * every separator first, before ever measuring a component, is what
 * makes this loop never hand mkdir() an empty ("") path the way the
 * old byte-by-byte '/' -> '\0' rewrite did for any leading slash
 * (mkdir-p-absolute-path-leading-slash, fixes/91). A component is
 * created by temporarily NUL-terminating 'dir' right after it --
 * mkdir(dir.s, ...) then sees the whole prefix built so far, leading
 * separator included, since nothing before that point is ever
 * touched.
 * ----------------------------------------------------------------------- */
static int
mkdir_parents(char* argv[], stralloc* dir, int verbose) {
  char* p = dir->s;

  while(*p) {
    char save;
    size_t len;

    while(*p == PATHSEP_C)
      p++;

    if(!*p)
      break;

    len = path_len_s(p);
    p += len;
    save = *p;
    *p = '\0';

    if(mkdir(dir->s, 0755) == -1 && errno != EEXIST) {
      builtin_error(argv, dir->s);
      return 1;
    }

    if(verbose) {
      buffer_putm_internal(fd_out->w, "mkdir '", dir->s, "'", 0);
      buffer_putnlflush(fd_out->w);
    }

    *p = save;
  }

  return 0;
}

/* output stuff
 * ----------------------------------------------------------------------- */
int
builtin_mkdir(int argc, char* argv[]) {
  int c, components = 0, verbose = 0;
  stralloc dir;
  char* d;

  /* check options */
  while((c = shell_getopt(argc, argv, "pv")) > 0) {
    switch(c) {
      case 'p': components = 1; break;
      case 'v': verbose = 1; break;

      default: builtin_invopt(argv); return 1;
    }
  }

  stralloc_init(&dir);

  while((d = argv[shell_optind++])) {
    stralloc_copys(&dir, d);
    stralloc_nul(&dir);

    if(components) {
      if(mkdir_parents(argv, &dir, verbose)) {
        stralloc_free(&dir);
        return 1;
      }

      continue;
    }

    if(mkdir(dir.s, 0755) == -1) {
      builtin_error(argv, dir.s);
      stralloc_free(&dir);
      return 1;
    }

    if(verbose) {
      buffer_putm_internal(fd_out->w, "mkdir '", dir.s, "'", 0);
      buffer_putnlflush(fd_out->w);
    }
  }

  stralloc_free(&dir);
  return 0;
}
