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

#define path_is_absolute(p, len) (path_rootlen((p), (len)) & 1)
#define path_is_absolute_s(p) (path_rootlen_s((p)) & 1)

const char* path_basename(const char*, size_t* baselen);
int path_canonicalize(const char*, stralloc*, int symbolic);
size_t path_dirlen(const char*, size_t);
int path_fnmatch(const char* pattern, size_t plen, const char* string, size_t slen, int flags);
int path_getcwd(stralloc*);
char* path_gethome(int uid);
int path_getsep(const char*);
size_t path_len(const char*, size_t);
size_t path_len_s(const char*);
int path_readlink(const char*, stralloc*);
int path_realpath(const char*, stralloc*, int symbolic, stralloc* cwd);
int path_rootlen(const char*, size_t);
int path_rootlen_s(const char*);

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

#endif
