#include <limits.h>

#include "str.h"
#include "path.h"
#include "stralloc.h"
#include "windoze.h"

/* separator predicate; native Windows takes '/' as well as '\\' */
#if WINDOWS
#define PATHSEP_S_MIXED "\\/"
#define path_issep(c) ((c) == '/' || (c) == '\\')
#else
#define PATHSEP_S_MIXED "/"
#define path_issep(c) ((c) == '/')
#endif

/* drive prefix "C:" (WINDOWS only) */
#if WINDOWS
#define path_isdrive(p) ((((p)[0] >= 'a' && (p)[0] <= 'z') || ((p)[0] >= 'A' && (p)[0] <= 'Z')) && (p)[1] == ':')
#else
#define path_isdrive(p) 0
#endif

/* short-circuits, so "" and "a" are never read past their terminator */
#define path_isabs(p) (path_issep((p)[0]) || (path_isdrive(p) && path_issep((p)[2])))
#define path_isrel(p) (!path_isabs(p))
/* a bare name, no separator */
#define path_isname(p) ((p)[path_len_s(p)] == '\0')
