/**
 * @defgroup   path
 * @brief      PATH module.
 * @{
 */
#ifndef _PATH_H__
#define _PATH_H__

#include "windoze.h"
#include "stralloc.h"
#include "str.h"

#include <limits.h>

typedef struct {
  size_t sz1, sz2;
} SizePair;

#if WINDOWS_NATIVE
#define PATHSEP_C '\\'
#define PATHSEP_S "\\"
#define PATHLISTSEP_C ';'
#define PATHLISTSEP_S ";"
#else
#define PATHSEP_C '/'
#define PATHSEP_S "/"
#define PATHLISTSEP_C ':'
#define PATHLISTSEP_S ":"
#endif

#define PATH_FNM_NOMATCH 1
#define PATH_FNM_PATHNAME (1 << 0) /* No wildcard can ever match /'.  */
#define PATH_FNM_NOESCAPE \
  (1 << 1)                       /* Backslashes don't quote special chars. \
                                  */
#define PATH_FNM_PERIOD (1 << 2) /* Leading .' is matched only explicitly.  */

char* path_basename(const char* path);
int path_canonicalize(const char* path, stralloc* sa, int symbolic);
int path_canonical_sa(stralloc* sa);
int path_canonical(const char* path, stralloc* out);
size_t path_collapse(char* path, size_t n);
int path_fnmatch(
    const char* pattern, unsigned int plen, const char* string, unsigned int slen, int flags);
void path_getcwd(stralloc* sa);
char* path_gethome(int uid);
int path_getsep(const char* path);
int path_is_absolute_b(const char* x, size_t n);
int path_is_absolute(const char* p);
size_t path_len_s(const char* s);
int path_readlink(const char* path, stralloc* sa);
int path_realpath(const char* path, stralloc* sa, int symbolic, stralloc* cwd);
size_t path_right(const char* s, size_t n);

#ifdef STRLIST_H
#endif

#ifndef PATH_MAX
#if WINDOWS
#include <windows.h>
#endif
#endif

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif

#endif /* _PATH_H__ */
/** @} */
