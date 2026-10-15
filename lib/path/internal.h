/* lib/path/internal.h: private helpers of lib/path/*.c; nothing outside lib/path/ includes it.
 * Every macro takes a view (p, n), never reads past n, and evaluates its arguments more than
 * once: pass plain lvalues. Upper-case and without the path_ prefix, so a private name never
 * collides with the public path_* functions of ../path.h.
 *
 *   SEP(c)                  c is a separator ('/', also '\\' on WINDOWS)
 *   SEPCHARS, NSEP          the separator set as a string, and its length
 *   DRIVE(p, n)             2 if p starts with "X:" (WINDOWS only), else 0
 *   ISDOT / ISDOTDOT        the whole view is "." / ".."
 *   ISDOTSEP / ISDOTDOTSEP  "." / ".." followed by a separator or the end
 *   ISABS / ISREL           the root contains a separator ("/x", "//x", "C:/x") / does not
 *   SAMEROOT(a, ra, b, rb)  the roots a[0..ra), b[0..rb) are the same root (off Windows: both or neither;
 *                           "//" is "/")
 *   SEPLEN / COMLEN         run of separators / of non-separators at p
 *   RSEPLEN / RCOMLEN       the same, ending at p + n
 * ----------------------------------------------------------------------- */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "../byte.h"
#include "../str.h"
#include "../path.h"
#include "../stralloc.h"
#include "../scan.h"
#include "../windoze.h"

#include <limits.h>

#if WINDOWS
#define SEPCHARS "\\/"
#define NSEP 2
#define SEP(c) ((c) == '/' || (c) == '\\')
#define DRIVE(p, n) (((n) >= 2 && !str_contains(SEPCHARS ".", (p)[0]) && (p)[1] == ':') ? 2 : 0)
#else
#define SEPCHARS "/"
#define NSEP 1
#define SEP(c) ((c) == '/')
#define DRIVE(p, n) 0
#endif

#if WINDOWS
#define SAMEROOT(a, ra, b, rb) ((ra) == (rb) && !byte_diff((a), (ra), (b)))
#else
#define SAMEROOT(a, ra, b, rb) (!(ra) == !(rb))
#endif

#define ISDOT(p, n) ((n) == 1 && (p)[0] == '.')
#define ISDOTSEP(p, n) ((n) >= 1 && (p)[0] == '.' && ((n) == 1 || SEP((p)[1])))
#define ISDOTDOT(p, n) ((n) == 2 && (p)[0] == '.' && (p)[1] == '.')
#define ISDOTDOTSEP(p, n) ((n) >= 2 && (p)[0] == '.' && (p)[1] == '.' && ((n) == 2 || SEP((p)[2])))

#define ISABS(p, n) (((n) >= 1 && SEP((p)[0])) || ((n) >= 3 && DRIVE(p, n) && SEP((p)[2])))
#define ISREL(p, n) (!ISABS(p, n))

#define SEPLEN(p, n) scan_charsetnskip((p), SEPCHARS, (n))
#define RSEPLEN(p, n) scan_charsetnrskip((p), SEPCHARS, (n))
#define COMLEN(p, n) scan_noncharsetnskip((p), SEPCHARS, (n))
#define RCOMLEN(p, n) scan_noncharsetnrskip((p), SEPCHARS, (n))

/* the symlink-following worker of path_resolve() (path_canonicalize.c) */
int path_canonicalize(const char* path, stralloc* sa, int symbolic);
