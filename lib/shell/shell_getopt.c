#include "../shell.h"
/* Based on dietlibc fnmatch (C) by Felix Leitner
 *
 * rewritten to support multiple runs
 */

#include "../str.h"

struct optstate shell_opt;

int
shell_getopt(int argc, char* const argv[], const char* optstring) {
  return shell_getopt_r(&shell_opt, argc, argv, optstring);
}

#define optarg state->arg
#define optidx state->idx
#define optind state->ind
#define optofs state->ofs
#define optopt state->opt

int
shell_getopt_r(struct optstate* state, int argc, char* const argv[], const char* optstring) {
  unsigned int offset;

  /* no-one will get shot for setting optind to 0 in libshish.a :) */
  if(optind == 0) {
    optind = 1;
    optidx = 0;
    optofs = 0;
  }

  for(;;) {
    /* are we finished? */
    if(optind > argc || !argv[optind] || *argv[optind] != '-' || argv[optind][1] == 0)
      return -1;

    /* ignore a trailing - */
    if(argv[optind][1] == '-' && argv[optind][2] == '\0') {
      optind++;
      return -1;
    }

    /* if we're just starting then initialize local static vars */
    if(optidx != optind) {
      optidx = optind;
      optofs = 0;
    }

    /* get next option char */
    optopt = argv[optind][optofs + 1];
    offset = str_chr(optstring, optopt);

    /* end of argument, continue on next one */
    if(optopt == '\0') {
      optind++;
      continue;
    }

    /* if the option char isn't in optstring we return '?' */
    if(optstring[offset] == '\0') {
      optind++;
      return '?';
    }

    /* argument expected? */
    if(optstring[offset + 1] == ':') {
      /* "-foo", return "oo" as optarg */
      if(optstring[offset + 2] == ':' || argv[optind][optofs + 2]) {
        if(!*(optarg = &argv[optind][optofs + 2]))
          optarg = 0;

        break;
      }

      optarg = argv[optind + 1];

      /* missing argument? */
      if(optarg == 0) {
        optind++;
        return ':';
      }

      optind++;
    } else {
      /* no argument */
      optofs++;
      return optopt;
    }
  }

found:
  optind++;
  return optopt;
}