#include "../builtin.h"
#include "../fd.h"
#include "../../lib/shell.h"
#include "../../lib/str.h"
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

/* output stuff
 * ----------------------------------------------------------------------- */
int
builtin_mkdir(int argc, char** argv) {
  int c, ret;
  stralloc dir;
  int components = 0, verbose = 0;
  char* d;
  size_t i;

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
      for(i = 0; i < dir.len; i++) {
        if(dir.s[i] == '/')
          dir.s[i] = '\0';
      }
    }
    for(i = 0; i < dir.len;) {
      ret = mkdir(dir.s, 0755);
      if(ret == -1) {
        if(components && errno == EEXIST) {
          errno = 0;
        } else {
          builtin_error(argv, dir.s);
          return 1;
        }
      }
      if(verbose) {
        buffer_putm_internal(fd_out->w, "mkdir '", dir.s, "'", 0);
        buffer_putnlflush(fd_out->w);
      }
      i += str_len(&dir.s[i]);
      dir.s[i] = '/';
    }
  }
  return 0;
}
