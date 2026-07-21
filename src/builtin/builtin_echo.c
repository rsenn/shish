#include "../builtin.h"
#include "../fdtable.h"
#include "../../lib/str.h"
#include "../../lib/scan.h"
#include "../../lib/alloc.h"

/* output stuff
 * ----------------------------------------------------------------------- */
int
builtin_echo(int argc, char* argv[]) {
  int i, nonl = 0, eval = 0, optind = 1;

  /* echo doesn't have real options in the getopt sense: any word
     starting with "-" that isn't made up entirely of n/e/E letters
     ends option parsing right there and is itself the first operand,
     printed as-is -- there's no such thing as an "invalid option" to
     echo (POSIX/dash/bash agree; "echo ---marker---" must print
     "---marker---", not error out) */
  for(; optind < argc; optind++) {
    const char* arg = argv[optind];
    size_t j;

    if(arg[0] != '-' || arg[1] == '\0')
      break;

    for(j = 1; arg[j]; j++)
      if(arg[j] != 'n' && arg[j] != 'e' && arg[j] != 'E')
        break;

    if(arg[j])
      break;

    for(j = 1; arg[j]; j++) {
      switch(arg[j]) {
        case 'n': nonl = 1; break;
        case 'e': eval = 1; break;
        case 'E': eval = 0; break;
      }
    }
  }

  for(i = optind; i < argc; i++) {
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
