#include "../builtin.h"
#include "../fd.h"
#include "../../lib/shell.h"

/* output stuff
 * ----------------------------------------------------------------------- */
int
builtin_echo(int argc, char* argv[]) {
  int c;
  int nonl = 0;
  int eval = 0;

  /* check options */
  while((c = shell_getopt(argc, argv, ":ne")) > 0) {
    switch(c) {
      case 'n': nonl = 1; break;
      case 'e': eval = 1; break;
      default: builtin_invopt(argv); return 1;
    }
  }

  /* TODO*/
  (void)eval;

  for(argv += shell_optind; *argv;) {
    buffer_puts(fd_out->w, *argv);

    if(*++argv)
      buffer_putspace(fd_out->w);
  }

  if(nonl)
    buffer_flush(fd_out->w);
  else
    buffer_putnlflush(fd_out->w);

  return 0;
}
