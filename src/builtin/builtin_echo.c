#include "../builtin.h"
#include "../fdtable.h"
#include "../../lib/str.h"
#include "../../lib/scan.h"
#include "../../lib/shell.h"
#include "../../lib/alloc.h"

/* output stuff
 * ----------------------------------------------------------------------- */
int
builtin_echo(int argc, char* argv[]) {
  int c, i, nonl = 0, eval = 0;

  /* check options */
  while((c = shell_getopt(argc, argv, "neE")) > 0) {
    switch(c) {
      case 'n': nonl = 1; break;
      case 'e': eval = 1; break;
      case 'E': eval = 0; break;
      default: builtin_invopt(argv); return 1;
    }
  }

  /* TODO*/
  (void)eval;

  for(i = shell_optind; i < argc; i++) {
    const char* arg = argv[i];
    size_t len = str_len(arg);
    char* s = eval ? alloc(len) : str_dup(arg);

    if(eval) {
      size_t k = 0, j;

      for(j = 0; j < len; j++) {
        int ch = arg[j];

        if(ch == '\\') {
          size_t n = 1;

          switch(arg[j + 1]) {
            case 'x':
              if((n = scan_xchar(&arg[j + 2], (unsigned char*)&ch)) > 0)
                ++n;
              break;

            case '0': n = scan_8int(&arg[j + 1], &ch); break;

            case 'E':
            case 'e': ch = '\033'; break;
            case 'f': ch = '\014'; break;
            case 'n': ch = '\n'; break;
            case 'r': ch = '\r'; break;
            case 't': ch = '\t'; break;
            case 'v': ch = '\v'; break;
            case 'b': ch = '\010'; break;
            case '\\': ch = '\\'; break;
            default: n = 0; break;
          }

          if(n > 0) {
            s[k++] = ch;
            j += n;
            continue;
          }
        }

        s[k++] = ch;
      }

      len = k;
    }

    buffer_put(fd_out->w, s, len);

    alloc_free(s);

    if(i + 1 < argc)
      buffer_putspace(fd_out->w);
  }

  if(nonl)
    buffer_flush(fd_out->w);
  else
    buffer_putnlflush(fd_out->w);

  return 0;
}
