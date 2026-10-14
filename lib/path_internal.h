#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "byte.h"
#include "str.h"
#include "path.h"
#include "stralloc.h"
#include "scan.h"
#include "windoze.h"

#include <limits.h>

/* separator predicate; native Windows takes '/' as well as '\\' */
#if WINDOWS
#define PATHSEP_CHARS "\\/"
#define PATHSEP_LENGTH 2
#define path_issep(c) ((c) == '/' || (c) == '\\')
#else
#define PATHSEP_CHARS "/"
#define PATHSEP_LENGTH 1
#define path_issep(c) ((c) == '/')
#endif

/* drive prefix "C:" (WINDOWS only) */
#if WINDOWS
#define path_isdriveb(p, n) (((n) >= 2 && !str_contains(PATHSEP_CHARS ".", (p)[0]) && (p)[1] == ':') ? 2 : 0)
#define path_isdrive(p) (((p)[0] && !str_contains(PATHSEP_CHARS ".", (p)[0]) && (p)[1] == ':') ? 2 : 0)
#else
#define path_isdriveb(p, n) 0
#define path_isdrive(p) 0
#endif

#define path_isdotb(p, n) ((n) == 1 && (p)[0] == '.')
#define path_isdotsepb(p, n) ((n) >= 1 && (p)[0] == '.' && ((n) == 1 || path_issep((p)[1])))

#define path_isdotdotb(p, n) ((n) == 2 && (p)[0] == '.' && (p)[1] == '.')
#define path_isdotdot(p) ((p)[0] == '.' && (p)[1] == '.' && ((p)[2] == '\0' || path_issep((p)[2])))

#define path_isdotdotsepb(p, n) ((n) >= 2 && (p)[0] == '.' && (p)[1] == '.' && ((n) == 2 || path_issep((p)[2])))

/* path_isabsb(_s) returns 1 if the path is absolute */
#define path_isabsb(p, n) (((n) >= 1 && path_issep((p)[0])) || ((n) >= 3 && path_isdriveb(p, n) && path_issep((p)[2])))
#define path_isabs(p) (path_issep((p)[0]) || (path_isdrive(p) && path_issep((p)[2])))

/* path_isrelb(_s) returns 1 if the path is relative */
#define path_isrelb(p, n) (!path_isabsb(p, n))
#define path_isrel(p) (!path_isabs(p))

/* path_seplen(_s) returns the length of the path separator */
#define path_seplen(x, n) scan_charsetnskip((x), PATHSEP_CHARS, (n))
#define path_rseplen(x, n) scan_charsetnrskip((x), PATHSEP_CHARS, (n))

/* path_comlen(_s) returns the length of the path component */
#define path_comlen(x, n) scan_noncharsetnskip((x), PATHSEP_CHARS, (n))
#define path_rcomlen(x, n) scan_noncharsetnrskip((x), PATHSEP_CHARS, (n))

#define path_component(s) str_chrs((s), PATHSEP_CHARS, PATHSEP_LENGTH)

#define path_len(fn, buf, len) fn((buf) , (len))
#define path_plen(fn, buf, len, root) fn((buf) + (root), (len) - (root))

/* stralloc variants */
#define path_lensa(fn, sa) fn((sa)->s, (sa)->len)
#define path_plensa(fn, sa, root) fn((sa)->s + (root), (sa)->len - (root))