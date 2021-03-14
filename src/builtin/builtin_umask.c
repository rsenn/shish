#include "../builtin.h"
#include "../sh.h"
#include "../fd.h"
#include "../../lib/fmt.h"
#include "../../lib/scan.h"

/* umask built-in
 *
 * ----------------------------------------------------------------------- */
int
builtin_umask(int argc, char* argv[]) {
  int c;
  int symbolic = 0, print = 0;
  char** argp;

  /* check options, -n for unexport, -p for output */
  while((c = shell_getopt(argc, argv, "pS")) > 0) {
    switch(c) {
      case 'p': print = 1; break;
      case 'S': symbolic = 1; break;
      default: builtin_invopt(argv); return 1;
    }
  }

  /* print umask, suitable for re-input */

  if(shell_optind < argc) {
    unsigned short num;

    if(scan_8short(argv[shell_optind], &num) > 0)
      sh->umask = num;
  } else {
    char buf[FMT_8LONG];
    size_t n = fmt_8long(buf, sh->umask);

    if(print)
      buffer_puts(fd_out->w, "umask ");
    if(n < 4)
      buffer_putnc(fd_out->w, '0', 4 - n);
    buffer_put(fd_out->w, buf, n);
    buffer_putnlflush(fd_out->w);

    return 0;
  }

  return 0;
}
