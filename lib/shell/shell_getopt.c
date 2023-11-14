#include "../shell.h"
/* Based on dietlibc fnmatch (C) by Felix Leitner
 *
 * rewritten to support multiple runs
 */

#include "../str.h"

static const char default_prefixes[] = {'-', 0};
struct optstate shell_opt = {default_prefixes, 0, 0, 0, 0, 0};

static void
state_clear(struct optstate* st) {
  st->ind = 0;
  st->ofs = 0;
  st->opt = '\0';
  st->arg = 0;
  st->prefixes = default_prefixes;
}

int
shell_getopt(int argc, char* const argv[], const char* optstring) {

  return shell_getopt_r(&shell_opt, argc, argv, optstring);
}

#define optind state->ind
#define optofs state->ofs
#define optopt state->opt
#define optarg state->arg
#define prefix state->prefix
#define prefixchars state->prefixes

int
shell_getopt_r(struct optstate* state, int argc, char* const argv[], const char* optstring) {
  unsigned int offset;

  if(optind == 0) {
    state_clear(state);

    if(optstring[0] == '+') {
      prefixchars = "+-";
      optstring++;
    } else {
      prefixchars = default_prefixes;
    }
    optind++;
  }

again:
  /* are we finished? */
  if(optind > argc || !argv[optind])
    return -1;

  offset = str_chr(prefixchars, argv[optind][0]);

  /* are we finished? */
  if(!(prefix = prefixchars[offset]) || argv[optind][1] == 0)
    return -1;

  /* ignore a trailing - */
  if(argv[optind][1] == '-' && argv[optind][2] == '\0') {
    optind++;
    return -1;
  }

  /* get next option char */
  optopt = argv[optind][optofs + 1];
  offset = str_chr(optstring, optopt);

  /* end of argument, continue on next one */
  if(optopt == '\0') {
    optind++;
    optofs = 0;
    goto again;
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

      goto found;
    }

    optarg = argv[optind + 1];

    /* missing argument? */
    if(optarg == 0) {
      optind++;
      return ':';
    }

    optind++;
  }
  /* no argument */
  else {
    optofs++;
    return optopt;
  }

found:
  optind++;
  return optopt;
}
