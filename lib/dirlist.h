#ifndef DIRLIST_H
#define DIRLIST_H

#include "arena.h"
#include "str.h"

/* One directory read whole into one packed blob, built in the caller's arena.
 *
 *   record = type byte, name, NUL        (2 bytes of overhead per entry)
 *
 *   dirlist_foreach(p, dl)
 *     use(dirlist_type(p), dirlist_name(p));
 *
 * Names are UTF-8 on every platform. The blob lives until the arena is
 * rewound or freed; there is no per-entry allocation and nothing to close.
 * ----------------------------------------------------------------------- */

/* entry types: our own, so no DT_* is needed (mingw and some libcs lack them) */
enum { DL_UNKNOWN = 0, DL_REG, DL_DIR, DL_LNK, DL_OTHER };

#define DIRLIST_DOTS 1     /* keep "." and ".." */
#define DIRLIST_SORT 2     /* sort by name with str_diff() */
#define DIRLIST_NOHIDDEN 4 /* drop every name starting with '.', "." and ".." included */

struct dirlist {
  char* s;    /* the records; NULL when the read failed (errno is set) */
  size_t len; /* bytes in s */
  size_t n;   /* number of records */
};

#define dirlist_type(p) ((unsigned char)*(p))
#define dirlist_name(p) ((p) + 1)
#define dirlist_next(p) ((p) + 2 + str_len((p) + 1))

/* visit every record of struct dirlist d, p is a char* */
#define dirlist_foreach(p, d) for((p) = (d).s; (p) < (d).s + (d).len; (p) = dirlist_next(p))

/* read directory path; DL_UNKNOWN where the platform gives no type.
 *
 *   arena*       a      receives the blob
 *   const char*  path   UTF-8 directory name
 *   unsigned     flags  DIRLIST_DOTS | DIRLIST_SORT | DIRLIST_NOHIDDEN
 * ----------------------------------------------------------------------- */
struct dirlist dirlist_read(arena* a, const char* path, unsigned flags);

/* sort the records of d by name in place (heap sort, no recursion). Scratch
 * is taken from a and returned before the call ends. Returns 0, or -1 with
 * errno ENOMEM and d unchanged. cmp NULL = str_diff().
 * ----------------------------------------------------------------------- */
int dirlist_sort(arena* a, struct dirlist* d, int (*cmp)(const char*, const char*));

#endif
