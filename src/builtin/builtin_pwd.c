#include "builtin.h"
#include "fd.h"
#include "sh.h"
#include "shell.h"

/* print working directory
 * ----------------------------------------------------------------------- */
int
builtin_pwd(int argc, char** argv) {
  int c;
  int physical = 0;

  /* check options, -L for symlink, -P for physical path */
  while((c = shell_getopt(argc, argv, "LP")) > 0) {
    switch(c) {
      case 'P': physical++; break;
      case 'L': physical = 0; break;
      default: builtin_invopt(argv); return 1;
    }
  }

  /* if the cwd is physical and a physical path was requested then getcwd() */
  if(sh->cwdsym && physical) {
    stralloc sa;
    stralloc_init(&sa);
    shell_getcwd(&sa);
    buffer_putsa(fd_out->w, &sa);
    buffer_putnlflush(fd_out->w);
    stralloc_free(&sa);
    return 0;
  }

  /* ..otherwise directly print the path from the shell env */
  buffer_putsa(fd_out->w, &sh->cwd);
  buffer_putnlflush(fd_out->w);
  return 0;
}
