#include "../shell.h"
/* Based on dietlibc fnmatch (C) by Felix Leitner
 *
 * rewritten to support multiple runs
 */

#include "../str.h"

static const char default_prefixes[] = {'-', 0};
struct optstate shell_opt = {default_prefixes, 0, 0, 0, 0, 0, 0, 0};

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
  return shell_getopt_long_r(&shell_opt, argc, argv, optstring, 0);
}

int
shell_getopt_long(int argc, char* const argv[], const char* optstring, const char* longopts) {
  return shell_getopt_long_r(&shell_opt, argc, argv, optstring, longopts);
}

int
shell_getopt_r(struct optstate* state, int argc, char* const argv[], const char* optstring) {
  return shell_getopt_long_r(state, argc, argv, optstring, 0);
}

#define optind state->ind
#define optofs state->ofs
#define optopt state->opt
#define optarg state->arg
#define prefix state->prefix
#define prefixchars state->prefixes

/* "--name", "--name=value" or "--name value" at argv[optind]; longopts is "name=X name:=X name?=X ..."
 *
 *   name=X   flag, returns X              name:=X  required argument     name?=X  optional argument (--name=value only)
 *
 * --help is known to every caller; a longopts word "help" overrides it.
 * ----------------------------------------------------------------------- */
static int
getopt_long(struct optstate* state, char* const argv[], const char* longopts) {
  const char* name = argv[optind] + 2;
  size_t nl = str_chr(name, '=');
  const char* w = longopts;

  state->lname = argv[optind];

  while(w && *w) {
    size_t l = str_chr(w, ' ');
    size_t n = 0;

    while(n < l && w[n] != '=' && w[n] != ':' && w[n] != '?')
      n++;

    if(n == nl && !str_diffn(w, name, nl)) {
      char mode = w[n] == '=' ? 0 : w[n];
      int code = (unsigned char)w[n + (mode ? 2 : 1)];
      int hasval = name[nl] == '=';

      optopt = code;
      optarg = 0;

      if(mode == 0 && hasval) {
        state->err = SHELL_OPTERR_NOARG;
        optind++;
        return '?';
      }

      if(mode && hasval) {
        optarg = (char*)name + nl + 1;
      } else if(mode == ':') {
        if(!(optarg = argv[optind + 1])) {
          state->err = SHELL_OPTERR_MISSING;
          optind++;
          return ':';
        }

        optind++;
      }

      optind++;
      return code;
    }

    w += l;
    w += *w == ' ';
  }

  optind++;

  if(nl == 4 && !str_diffn(name, "help", 4) && !name[4]) {
    state->err = SHELL_OPTERR_HELP;
    return SHELL_OPT_HELP;
  }

  state->err = SHELL_OPTERR_UNKNOWN;
  return '?';
}

int
shell_getopt_long_r(struct optstate* state, int argc, char* const argv[], const char* optstring, const char* longopts) {
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

  state->err = 0;
  state->lname = 0;

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

  /* a long option; "+" callers (set, sh) keep their own meaning of "--name" */
  if(prefixchars == default_prefixes && optofs == 0 && argv[optind][1] == '-')
    return getopt_long(state, argv, longopts);

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
    state->err = SHELL_OPTERR_UNKNOWN;
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
      state->err = SHELL_OPTERR_MISSING;
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
