#include "../builtin.h"
#include "../fd.h"
#include "../term.h"
#include "../../lib/shell.h"
#include "../../lib/fmt.h"
#include "../../lib/str.h"

/* output stuff
 * ----------------------------------------------------------------------- */
int
builtin_cat(int argc, char** argv) {
  int c, ret = 0;
  int number_lines = 0, number_nonempty = 0;
  char* arg;
  ssize_t n, line = 1;
  buffer inb;
  buffer* in;
  /* check options */
  while((c = shell_getopt(argc, argv, "nb")) > 0) {
    switch(c) {
      case 'n': number_lines = 1; break;
      case 'b': number_nonempty = 1; break;
      default: builtin_invopt(argv); return 1;
    }
  }
  if(argv[shell_optind] == NULL) {
    argv[shell_optind] = "-";
    argc++;
  }
  while((arg = argv[shell_optind])) {
    char buf[1024];
    /*   buffer_putm_internal(fd_err->w, "File: '", arg, "'\n", 0);
       buffer_flush(fd_err->w);
   */
    if(!str_diff(arg, "-")) {
      in = fd_in->r;
    } else {
      in = &inb;
      if(buffer_mmapread(in, arg)) {
        builtin_error(argv, arg);
        ret = 1;
        break;
      }
    }
    for(;;) {
      ret = buffer_get_until(in, buf, sizeof(buf), "\r\n", 2);
      if(ret == 0) {
        if(in->op == (buffer_op_proto*)&term_read) {
          buffer_puts(fd_err->w, "EOF");
          buffer_putnlflush(fd_err->w);
        }
        goto next;
      }
      if(ret > 0) {
        char eol = buf[ret - 1];
        if(number_lines) {
          char buf[FMT_ULONG];
          n = fmt_ulong(buf, line);
          if(n < 5)
            buffer_putnspace(fd_out->w, 5 - n);
          buffer_put(fd_out->w, buf, n);
          buffer_putspace(fd_out->w);
        }
        buffer_put(fd_out->w, buf, ret);
        buffer_flush(fd_out->w);
        if(eol == '\n')
          line++;
      }
    }
  next:
    if(++shell_optind == argc)
      break;
  }
  return ret;
}
