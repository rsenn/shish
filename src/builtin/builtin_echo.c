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
  while((c = shell_getopt(argc, argv, "ne")) > 0) {
    switch(c) {
      case 'n': nonl = 1; break;
      case 'e': eval = 1; break;
      default: builtin_invopt(argv); return 1;
    }
  }

  /* TODO*/
  (void)eval;

  for(i = shell_optind; i < argc; i++) {
    size_t j, len = str_len(argv[i]);
    char* s = alloc(len + 1);

    for(j = 0; j < len; j++) {
      if(argv[i][j] == '\\') {
        size_t n = 0;
        unsigned char ch = '\0';

        if(argv[i][j + 1] == 'x') {
          n = scan_xchar(&argv[i][j + 2], &ch);

          if(n > 0)
            ++n;

        } else if(argv[i][j + 1] == '0') {

          n = scan_8int(&argv[i][j + 1], &ch);
        }

        if(n > 0) {
          s[j] = ch;
          j += n;
          continue;
        }
      }

      s[j] = argv[i][j];
    }

    buffer_put(fd_out->w, s, j);

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
