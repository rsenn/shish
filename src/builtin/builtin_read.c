#include "../builtin.h"
#include "../fd.h"
#include "../expand.h"
#include "../var.h"
#include "../debug.h"
#include "../../lib/scan.h"
#include "../../lib/str.h"
#include "../../lib/fmt.h"
#include "../../lib/buffer.h"
#include <math.h>

/* read built-in
 *
 * ----------------------------------------------------------------------- */
int
builtin_read(int argc, char* argv[]) {
  int c, raw = 0, silent = 0, nchars = -1, fd = 0;
  char** argp;
  int num_args;
  const char *delim = "\n", *prompt = 0;
  size_t ndelims;
  double timeout = -1;
  stralloc data;
  int index;

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
  num_args = argc - shell_optind;

  for(index = 0; index < num_args; index++) {
    if(!var_valid(argp[index])) {
      builtin_errmsg(argv, argp[index], "not a valid identifier");
      return 1;
    }
  }

  if(delim)
    ndelims = str_len(delim);


  if(prompt) 
    buffer_putsflush(fd_out->w, prompt);
  

  {
    int ret, status = 0;
    const char* ifs;
    size_t ifslen;
    char *ptr, *end;
    index = 0;

    stralloc_init(&data);

    if(delim) {
      if((ret = buffer_get_token_sa(fd_in->r, &data, delim, ndelims)) > 0) {
        stralloc_trimr(&data, "\r\n", 2);
        stralloc_nul(&data);
      } else {
        status = 1;
      }
    }

    ifs = var_vdefault("IFS", IFS_DEFAULT, &ifslen);

    for(ptr = stralloc_begin(&data), end = stralloc_end(&data); ptr < end;) {
      size_t len;

      len = scan_charsetnskip(ptr, ifs, end - ptr);

      if((ptr += len) >= end)
        break;

      len = index + 1 == num_args ? end - ptr : scan_noncharsetnskip(ptr, ifs, end - ptr);

      if(!var_valid(argp[index]))
        builtin_errmsg(argv, argp[index], "not a valid identifier");
      else
        var_setv(argp[index], ptr, len, 0);
      index++;
      ptr += len;
    }

    while(index < num_args) {
      if(!var_valid(argp[index]))
        builtin_errmsg(argv, argp[index], "not a valid identifier");
      else
        var_set(argp[index], 0);
      index++;
    }

    /* set each argument */
    for(; *argp; argp++) {
    }

    return status;
  }
}
