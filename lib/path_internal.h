#include <limits.h>

#include "str.h"
#include "path.h"
#include "stralloc.h"
#include "scan.h"
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
#define path_isdrive(p) (isalnum((p)[0]) && (p)[1] == ':')
#else
#define path_isdrive(p) 0
#endif

/* short-circuits, so "" and "a" are never read past their terminator */
#define path_isabs(p) (path_issep((p)[0]) || (path_isdrive(p) && path_issep((p)[2])))
#define path_isrel(p) (!path_isabs(p))

#define path_drivelen(p, n) \
  ((n) >= 2 && (PATHSEP_S_MIXED ".")[str_chr(PATHSEP_S_MIXED ".", p[0])] == '\0' && p[1] == ':' ? 2 : 0)

#define path_seplen(p, n) scan_charsetnskip((p), PATHSEP_S_MIXED, (n))

#define path_comlen(p, n) scan_noncharsetnskip((p), PATHSEP_S_MIXED, (n))
