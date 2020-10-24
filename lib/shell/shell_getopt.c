#include "../shell.h"
/* Based on dietlibc fnmatch (C) by Felix Leitner
 *
 * rewritten to support multiple runs
 */

#include "../str.h"

int shell_optind = 1;
char shell_optopt;
char* shell_optarg;
int shell_optidx = 0;
int shell_optofs = 0;

int
shell_getopt(int argc, char* const argv[], const char* optstring) {
  unsigned int offset;

  /* no-one will get shot for setting optind to 0 in libshish.a :) */
  /*  if(shell_optind == 0)
    {
      shell_optind = 1;
      shell_optidx = 0;
      shell_optofs = 0;
    }*/
again:
  /* are we finished? */
  if(shell_optind > argc || !argv[shell_optind] || *argv[shell_optind] != '-' || argv[shell_optind][1] == 0)
    return -1;

  /* ignore a trailing - */
  if(argv[shell_optind][1] == '-' && argv[shell_optind][2] == '\0') {
    shell_optind++;
    return -1;
  }

  /* if we're just starting then initialize local static vars */
  if(shell_optidx != shell_optind) {
    shell_optidx = shell_optind;
    shell_optofs = 0;
  }

  /* get next option char */
  shell_optopt = argv[shell_optind][shell_optofs + 1];
  offset = str_chr(optstring, shell_optopt);

  /* end of argument, continue on next one */
  if(shell_optopt == '\0') {
    shell_optind++;
    goto again;
  }

  /* if the option char isn't in optstring we return '?' */
  if(optstring[offset] == '\0') {
    shell_optind++;
    return '?';
  }

  /* argument expected? */
  if(optstring[offset + 1] == ':') {
    /* "-foo", return "oo" as optarg */
    if(optstring[offset + 2] == ':' || argv[shell_optind][shell_optofs + 2]) {
      if(!*(shell_optarg = &argv[shell_optind][shell_optofs + 2]))
        shell_optarg = 0;

      goto found;
    }

    shell_optarg = argv[shell_optind + 1];

    /* missing argument? */
    if(shell_optarg == 0) {
      shell_optind++;
      return ':';
    }

    shell_optind++;
  }
  /* no argument */
  else {
    shell_optofs++;
    return shell_optopt;
  }

found:
  shell_optind++;
  return shell_optopt;
}
