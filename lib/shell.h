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
};

int shell_getopt_r(struct optstate*,
                   int argc,
                   char* const argv[],
                   const char* optstring);
int shell_getopt(int argc, char* const argv[], const char* optstring);

extern struct optstate shell_opt;

#define shell_optarg shell_opt.arg
#define shell_optind shell_opt.ind
#define shell_optofs shell_opt.ofs
#define shell_optopt shell_opt.opt
#define shell_optprefix shell_opt.prefix
#define shell_optprefixes shell_opt.prefixes

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

#endif /* SHELL_H */
