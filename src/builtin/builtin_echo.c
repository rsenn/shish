#include "../builtin.h"
#include "../fd.h"
#include "../../lib/shell.h"

/* output stuff
 * ----------------------------------------------------------------------- */
int
builtin_echo(int argc, char* argv[]) {
  int c, i, nonl = 0, eval = 0;

  /* check options */
  while((c = shell_getopt(argc, argv, "ne")) > 0) {
    switch(c) {
      case 'n': nonl = 1; break;
      case 'e': eval = 1; break;
      default: builtin_invopt(argv); return 1;
    }
  }

  /* TODO*/
  (void)eval;

  buffer_puts(fd_out->w, "echo: ");

  for(i = shell_optind; i < argc; i++) {
    buffer_puts(fd_out->w, argv[i]);

    if(i + 1 < argc)
      buffer_putspace(fd_out->w);
  }

  if(nonl)
    buffer_flush(fd_out->w);
  else
    buffer_putnlflush(fd_out->w);

  return 0;
}
