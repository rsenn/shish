#ifndef SHELL_H
#define SHELL_H

#include "buffer.h"
#include "stralloc.h"
#include <stdlib.h>

#define SH_FNM_NOMATCH 1
#define SH_FNM_PATHNAME (1 << 0) /* No wildcard can ever match /'.  */
#define SH_FNM_NOESCAPE (1 << 1) /* Backslashes don't quote special chars.  */
#define SH_FNM_PERIOD (1 << 2)   /* Leading .' is matched only explicitly.  */

void shell_getcwd(stralloc* sa);

int shell_realpath(const char* path, stralloc* sa, int symbolic, stralloc* cwd);

int shell_readlink(const char* path, stralloc* sa);

int shell_canonicalize(const char* path, stralloc* sa, int symbolic);

int shell_fnmatch(const char* pattern, unsigned int plen, const char* string, unsigned int slen, int flags);

char* shell_basename(char* path);
char* shell_dirname(char* path);

char* shell_gethome(int uid);
char* shell_gethostname(stralloc* sa);

struct optstate {
  const char* prefixes;
  int ind, ofs, opt;
  char* arg;
  char prefix;
};

int shell_getopt_r(struct optstate*, int argc, char* const argv[], const char* optstring);
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

#ifndef DEBUG_ALLOC
void* shell_alloc(unsigned long size);
void* shell_realloc(void* ptr, unsigned long size);
void* shell_strdup(const char* s);
#define shell_free(p) free((p))
#else
extern void debug_free(const char* file, unsigned int line, void* p);
void* shell_allocdebug(const char* file, unsigned int line, unsigned long size);
void* shell_reallocdebug(const char* file, unsigned int line, void* ptr, unsigned long size);
void* shell_strdupdebug(const char* file, unsigned int line, const char* s);
#define shell_alloc(n) shell_allocdebug(__FILE__, __LINE__, (n))
#define shell_realloc(p, n) shell_reallocdebug(__FILE__, __LINE__, (p), (n))
#define shell_strdup(s) shell_strdupdebug(__FILE__, __LINE__, (s))
#define shell_free(p) debug_free(__FILE__, __LINE__, (p))
#endif /* DEBUG_ALLOC */

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
