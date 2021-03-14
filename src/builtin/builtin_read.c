#include "../builtin.h"
#include "../fd.h"
#include "../expand.h"
#include "../var.h"
#include "../debug.h"
#include "../../lib/scan.h"
#include "../../lib/str.h"
#include <math.h>

/* read built-in
 *
 * ----------------------------------------------------------------------- */
int
builtin_read(int argc, char* argv[]) {
  int c, raw = 0, silent = 0, nchars = -1, fd = 0;
  char** argp;
  const char *delim = "\n", *prompt = 0;
  size_t ndelims, nfields;
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
  argp = &argv[shell_optind];
  nfields = argc - shell_optind;

  if(delim)
    ndelims = str_len(delim);

  {
    int ret, status = 0;
    const char* ifs;
    size_t ifslen;
    char *ptr, *end;

    stralloc_init(&line);

    if(delim) {
      ret = buffer_get_token_sa(fd_in->r, &line, delim, ndelims);

      if(ret > 0) {
        stralloc_trimr(&line, "\r\n", 2);
        stralloc_nul(&line);
      } else {
        status = 1;
      }

      debug_ulong("ret", ret, 0);
      debug_nl_fl();
    }

    debug_stralloc("line", &line, 0, '"');
    debug_nl_fl();

    ifs = var_vdefault("IFS", IFS_DEFAULT, &ifslen);

    for(ptr = stralloc_begin(&line), end = stralloc_end(&line); ptr < end;) {
      size_t i = scan_charsetnskip(ptr, ifs, ifslen);

      if((ptr += i) >= end)
        break;

      if(ptr + (i = scan_noncharsetnskip(ptr, ifs, ifslen)) > end)
        i = end - ptr;
    }

    /* set each argument */
    for(; *argp; argp++) {
      if(!var_valid(*argp)) {
        builtin_errmsg(argv, *argp, "not a valid identifier");
        continue;
      }
    }
    debug_ulong("status", status, 0);
    debug_nl_fl();
    return status;
  }
}
