#include "../builtin.h"
#include "../fd.h"
#include "../var.h"
#include "../../lib/scan.h"
#include "../../lib/str.h"
#include <math.h>

/* export built-in
 *
 * ----------------------------------------------------------------------- */
int
builtin_read(int argc, char* argv[]) {
  int c, raw = 0, silent = 0, nchars = -1, fd = 0;
  char** argp;
  const char *delim = "\n", *prompt = 0;
  double timeout = -1;
  stralloc line;

  /* check options, -p for output */
  while((c = shell_getopt(argc, argv, "d:n:N:p:rst:u:")) > 0) {
    switch(c) {
      case 'd': delim = shell_optarg; break;
      case 'N': delim = 0;
      case 'n': nchars = scan_int(shell_optarg, &nchars); break;
      case 'p': prompt = shell_optarg; break;
      case 'r': raw = 1; break;
      case 's': silent = 1; break;
      case 't': scan_double(shell_optarg, &timeout); break;
      case 'u': scan_int(shell_optarg, &fd); break;
      default: builtin_invopt(argv); return 1;
    }
  }

  stralloc_init(&line);

  if(delim)
    buffer_get_token_sa(fd_in->r, &line, delim, str_len(delim));

  argp = &argv[shell_optind];

  /* set each argument */
  for(; *argp; argp++) {
    if(!var_valid(*argp)) {
      builtin_errmsg(argv, *argp, "not a valid identifier");
      continue;
    }
  }

  return 0;
}
