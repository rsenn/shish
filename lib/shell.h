#ifndef SHELL_H
#define SHELL_H

#include <stdlib.h>
#include "buffer.h"
#include "stralloc.h"

#define SH_FNM_NOMATCH     1
#define SH_FNM_PATHNAME    (1 << 0) /* No wildcard can ever match /'.  */
#define SH_FNM_NOESCAPE    (1 << 1) /* Backslashes don't quote special chars.  */
#define SH_FNM_PERIOD      (1 << 2) /* Leading .' is matched only explicitly.  */

void shell_getcwd(stralloc *sa, unsigned long start);

int shell_realpath(const char *path, stralloc *sa, int symbolic, stralloc *cwd);

int shell_readlink(const char *path, stralloc *sa);
  
int shell_canonicalize(const char *path, stralloc *sa, int symbolic);

int shell_fnmatch(const char *pattern, unsigned int plen,
                  const char *string, unsigned int slen, int flags);  

char *shell_basename(char *path);
char *shell_dirname(char *path);

char *shell_gethome(int uid);
char *shell_gethostname(stralloc *sa);

int shell_getopt(int argc, char * const argv[], const char *optstring);  

extern int   shell_optind;
extern int   shell_optidx;
extern int   shell_optofs;
extern char  shell_optopt;
extern char *shell_optarg;

/* set the output buffer and the basename for error messages */
void shell_init(buffer *b, const char *n);
int shell_error(const char *s);
int shell_errorn(const char *s, unsigned int len);

extern buffer     *shell_buff;
extern const char *shell_name;

#ifndef DEBUG
void *shell_alloc(unsigned long size);
void *shell_realloc(void *ptr, unsigned long size);
void *shell_strdup(const char *s);
#define shell_free(p) free((p))  
#else
extern void debug_free(const char *file, unsigned int line, void *p);
void *shell_allocdebug(const char *file, unsigned int line, unsigned long size);
void *shell_reallocdebug(const char *file, unsigned int line, void *ptr, unsigned long size);
void *shell_strdupdebug(const char *file, unsigned int line, const char *s);
#define shell_alloc(n) shell_allocdebug(__FILE__, __LINE__, (n))
#define shell_realloc(p, n) shell_reallocdebug(__FILE__, __LINE__, (p), (n))
#define shell_strdup(s) shell_strdupdebug(__FILE__, __LINE__, (s))
#define shell_free(p) debug_free(__FILE__, __LINE__, (p))
#endif /* DEBUG */


#endif /* SHELL_H */
