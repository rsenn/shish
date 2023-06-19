#include "../builtin.h"
#include "../fdtable.h"
#include "../expand.h"
#include "../var.h"
#include "../debug.h"
#include "../term.h"
#include "../../lib/byte.h"
#include "../../lib/scan.h"
#include "../../lib/str.h"
#include "../../lib/fmt.h"
#include "../../lib/buffer.h"
#include <termios.h>

static int
any(int c) {
  return 1;
}

struct predicate_data {
  const char* delim;
  int ndelim;
  int nchars;
};

static int
predicate_function(stralloc* sa, void* ptr) {
  struct predicate_data* p = ptr;
  if(p->delim && p->ndelim > 0) {
    if(sa->len &&
       byte_chr(p->delim, p->ndelim, sa->s[sa->len - 1]) < (size_t)p->ndelim)
      return 1;
  }
  if(p->nchars > 0)
    return sa->len >= (size_t)p->nchars;

  return 0;
}
/* read built-in
 *
 * ----------------------------------------------------------------------- */
int
builtin_read(int argc, char* argv[]) {
  int c, raw = 0, silent = 0, fd = 0;
  char** argp;
  int num_args;
  const char* prompt = 0;
  // double timeout = -1;
  stralloc data;
  int index;
  struct predicate_data p = {"\n", 1, -1};

  while((c = shell_getopt(argc, argv, "d:n:N:p:rsu:")) > 0) {
    switch(c) {
      case 'd':
        p.delim = shell_optarg;
        p.ndelim = str_len(p.delim);
        break;
      case 'N': p.delim = 0; p.ndelim = 0;
      case 'n': scan_int(shell_optarg, &p.nchars); break;
      case 'p': prompt = shell_optarg; break;
      case 'r': raw = 1; break;
      case 's':
        silent = 1;
        break;
        // case 't': scan_double(shell_optarg, &timeout); break;
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
  if(prompt)
    buffer_putsflush(fd_out->w, prompt);

  {
    int ret, status = 0;
    const char* ifs;
    size_t len;
    char *ptr, *end;
    index = 0;
    struct termios attrs;
    buffer* input = fdtable[fd]->r;

    stralloc_init(&data);

    if(silent)
      term_attr(input->fd, 1, &attrs);

    if((ret = buffer_get_token_sa_pred(input, &data, predicate_function, &p)) >
       0) {
      stralloc_trimr(&data, "\r\n", 2);
    } else {
      status = 1;
    }

    if(silent)
      term_restore(input->fd, &attrs);

    ifs = var_vdefault("IFS", IFS_DEFAULT, &len);

    if(!raw)
      expand_unescape(&data, any);

    stralloc_nul(&data);

    ptr = stralloc_begin(&data);
    end = stralloc_end(&data);

    for(index = 0; index < num_args; index++) {

      if(ptr < end) {
        len = scan_charsetnskip(ptr, ifs, end - ptr);
        ptr += len;

        len = end - ptr;
        if(index < num_args - 1)
          len = scan_noncharsetnskip(ptr, ifs, len);

        var_setv(argp[index], ptr, len, 0);
        ptr += len;
        continue;
      }
      var_set(argp[index], 0);
    }
    return status;
  }
}
