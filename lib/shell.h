#ifndef SHELL_H
#define SHELL_H

#include "buffer.h"
#include "stralloc.h"
#include <stdlib.h>

#define SH_FNM_NOMATCH 1
#define SH_FNM_PATHNAME (1 << 0) /* No wildcard can ever match /'.  */
#define SH_FNM_NOESCAPE (1 << 1) /* Backslashes don't quote special chars.  */
#define SH_FNM_PERIOD (1 << 2)   /* Leading .' is matched only explicitly.  */

char* shell_dirname(char* path);

char* shell_gethostname(stralloc* sa);

struct optstate {
  const char* prefixes;
  int ind, ofs, opt;
  char* arg;
  char prefix;
  const char* lname; /* the "--name[=value]" word of the last long option, for messages */
  int err;           /* SHELL_OPTERR_* of the last '?' or ':' return */
};

/* shell_getopt_long: longopts is "name=X name:=X name?=X ...", see doc/getopt-long.md.
 * "--help" is known to every caller and returns SHELL_OPT_HELP. */
#define SHELL_OPT_HELP 0x100
#define SHELL_OPTERR_UNKNOWN 1
#define SHELL_OPTERR_MISSING 2
#define SHELL_OPTERR_NOARG 3
#define SHELL_OPTERR_HELP 4

int shell_getopt_long_r(struct optstate*, int argc, char* const argv[], const char* optstring, const char* longopts);
int shell_getopt_r(struct optstate*, int argc, char* const argv[], const char* optstring);
int shell_getopt_long(int argc, char* const argv[], const char* optstring, const char* longopts);
int shell_getopt(int argc, char* const argv[], const char* optstring);

extern struct optstate shell_opt;

#define shell_optarg shell_opt.arg
#define shell_optind shell_opt.ind
#define shell_optofs shell_opt.ofs
#define shell_optopt shell_opt.opt
#define shell_optprefix shell_opt.prefix
#define shell_optprefixes shell_opt.prefixes
#define shell_optlname shell_opt.lname
#define shell_opterr shell_opt.err

/* set the output buffer and the basename for error messages */
void shell_init(buffer* b, const char* n);
int shell_error(const char* s);
int shell_errorn(const char* s, unsigned int len);

extern buffer* shell_buff;
extern const char* shell_name;

#include <limits.h>

#ifndef PATH_MAX
#if WINDOWS_NATIVE
#include <windows.h>
#define PATH_MAX MAX_PATH
#endif
#endif

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#endif
