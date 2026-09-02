#include "../builtin.h"
#include "../fdtable.h"
#include "../sh.h"
#include "../../lib/str.h"
#include "../../lib/alloc.h"

/* output stuff
 * ----------------------------------------------------------------------- */
const char help_echo[] =
    "    Write arguments to standard output.\n"
    "\n"
    "    -n              suppress the trailing newline\n"
    "    -e              enable \\n \\t \\r \\v \\b \\f \\\\ \\0NNN \\xHH escapes\n"
    "    -E              disable those escapes (default)\n"
    "    arg             text to print, space-separated\n"
    "\n"
    "    A leading '-' operand not made up entirely of n/e/E letters\n"
    "    ends option parsing and is itself printed.\n";

int
builtin_echo(int argc, char* argv[]) {
  int i, nonl = 0, eval = 0, optind = 1;

  /* echo doesn't have real options in the getopt sense: any word
     starting with "-" that isn't made up entirely of n/e/E letters
     ends option parsing and is itself the first operand, printed
     as-is -- there's no such thing as an "invalid option" to echo. */
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

    if(eval)
      len = sh_unescape(arg, len, s);

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
