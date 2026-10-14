#ifndef _PATH_H__
#define _PATH_H__

#include "str.h"
#include "stralloc.h"
#include "windoze.h"

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

const char* path_basename(const char*, size_t, size_t* baselen);
size_t path_cat(char* out, size_t cap, const char* a, size_t an, const char* b, size_t bn);
int path_cat_sa(stralloc* out, const char* a, size_t an, const char* b, size_t bn);
size_t path_canon(char* out, size_t cap, const char* p, size_t n);
int path_canonicalize(const char*, stralloc*, int symbolic);
size_t path_abs(char* out, size_t cap, const char* p, size_t n, const char* cwd, size_t cwdn);
int path_abs_sa(stralloc* out, const char* p, size_t n);
size_t path_rel(char* out, size_t cap, const char* p, size_t pn, const char* base, size_t bn);
int path_rel_sa(stralloc* out, const char* p, size_t pn, const char* base, size_t bn);
size_t path_common(const char* a, size_t an, const char* b, size_t bn, size_t* blen);
size_t path_extoff(const char* p, size_t n);
int path_isname(const char* p, size_t n);
size_t path_dirlen(const char*, size_t);
int path_fnmatch(const char* pattern, size_t plen, const char* string, size_t slen, int flags);
int path_getcwd(stralloc*);
char* path_gethome(int uid);
int path_getsep(const char*);
int path_readlink(const char*, stralloc*);
int path_realpath(const char*, stralloc*, int symbolic, stralloc* cwd);
size_t path_lenb(const char* s, size_t n);
size_t path_rootlen(const char*, size_t);
SizePair path_commonlenb(const char*, size_t, const char*, size_t);
size_t path_commonlen(const char*, const char*);

#define path_len_s(s) path_lenb((s), str_len((s)))
#define path_rootlen_s(s) path_rootlen((s), str_len((s)))
#define path_basename_s(s) path_basename((s), str_len((s)), 0)

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
