#ifndef _PATH_H__
#define _PATH_H__

#include "str.h"
#include "stralloc.h"
#include "windoze.h"

#include <limits.h>

#if WINDOWS_NATIVE
#define PATHSEP_C '\\'
#define PATHSEP_S "\\"
#define PATHLISTSEP_C ';'
#define PATHLISTSEP_S ";"
#define PATHLISTSEP_CHARS ";"
#else
#define PATHSEP_C '/'
#define PATHSEP_S "/"
#define PATHLISTSEP_C ':'
#define PATHLISTSEP_S ":"
#define PATHLISTSEP_CHARS ":;"
#endif

#define PATH_FNM_NOMATCH 1
#define PATH_FNM_PATHNAME (1 << 0) /* No wildcard can ever match /'.  */
#define PATH_FNM_NOESCAPE \
  (1 << 1)                       /* Backslashes don't quote special chars. \
                                  */
#define PATH_FNM_PERIOD (1 << 2) /* Leading .' is matched only explicitly.  */

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
int path_isabs(const char* p, size_t n);
int path_isrel(const char* p, size_t n);
size_t path_next(const char* p, size_t n, size_t pos);
size_t path_parentlen(const char* p, size_t n);
size_t path_dirlen(const char*, size_t);
size_t path_listlen(const char* l, size_t n);
size_t path_listnext(const char* l, size_t n, size_t pos);
size_t path_search(char* out, size_t cap, const char* l, size_t ln, const char* name, size_t nl, int (*ok)(const char*, void*), void* arg);
int path_fnmatch(const char* pattern, size_t plen, const char* string, size_t slen, int flags);
int path_getcwd(stralloc*);
char* path_gethome(int uid);
int path_readlink(const char*, stralloc*);
int path_realpath(const char*, stralloc*, int symbolic, stralloc* cwd);
size_t path_rootlen(const char*, size_t);

/* PATH_LIST_FOREACH(l, n, elem, elen) visits every element of a PATH-style list, empty ones too
 * (an empty element is the current directory):  "a::b:" -> "a", "", "b", ""
 *
 *   const char*  l     the list           size_t  n     its length
 *   const char*  elem  receives the start  size_t  elen  receives the length
 * ----------------------------------------------------------------------- */
#define PATH_LIST_FOREACH(l, n, elem, elen) \
  for(size_t pl_pos_ = 0; pl_pos_ != (size_t)-1 && ((elem) = (l) + pl_pos_, (elen) = path_listlen((elem), (n)-pl_pos_), 1); pl_pos_ = path_listnext((l), (n), pl_pos_))

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
