#ifndef WALK_H
#define WALK_H

#include "arena.h"
#include "dirlist.h"
#include <sys/stat.h>

/* Walk the tree below one root, one visit per walk_read().
 *
 *   struct walk w;
 *   struct walk_ent* e;
 *
 *   if(walk_open(&w, "dir", WALK_POST | WALK_ERR))
 *     return -1;
 *   while((e = walk_read(&w)))
 *     use(e->phase, e->path, e->type);
 *   walk_close(&w);
 *
 * One descriptor is open at a time: a directory is read whole through
 * dirlist and closed before its first child is visited. The handle lives in
 * the caller's struct, so walks nest freely. Symlinks are never followed
 * unless WALK_LOGICAL (all) or WALK_COMFOLLOW (the root) is passed.
 * ----------------------------------------------------------------------- */

/* what a visit is (e->phase), and, as flags, which visits walk_open() asks for.
 * WALK_FILE (everything that is not a directory) is always reported. */
enum {
  WALK_FILE = 1,  /* not a directory: regular file, symlink (unless followed), ... */
  WALK_PRE = 2,   /* directory, before its children; walk_skip() leaves it unread */
  WALK_POST = 4,  /* directory, after its children (not after an error or skip) */
  WALK_ERR = 8,   /* e->err: root unreadable, directory unreadable (no WALK_POST) */
  WALK_CYCLE = 16 /* WALK_LOGICAL: directory already being walked above it; not entered */
};

/* policy, flags only */
enum {
  WALK_LOGICAL = 32,  /* follow every symlink (default: none) */
  WALK_COMFOLLOW = 64, /* follow the root if it is a symlink */
  WALK_XDEV = 128      /* a directory on another device is reported, not read */
};

struct walk_ent {
  const char* path; /* "root/a/b", valid until the next walk_read() */
  const char* rel;  /* "a/b": below the root, "" for the root itself */
  const char* name; /* last component; the whole root string for the root */
  size_t len;       /* str_len(path) */
  int depth;        /* 0 for the root */
  int phase;        /* WALK_FILE, WALK_PRE, ... */
  int err;          /* errno, for WALK_ERR */
  int type;         /* DL_*, never DL_UNKNOWN; the target's when followed */
  unsigned islink : 1; /* the entry is a symlink (type is DL_LNK unless followed) */
  unsigned kept : 1;   /* WALK_POST: walk_keep() was called below this directory */
};

struct walk_frame;

struct walk {
  struct walk_ent e;
  int maxdepth;    /* -1 = unlimited; set after walk_open(): deeper directories are not read */
  unsigned flags;
  arena la, pa, fa;      /* directory lists; path; frames */
  char* path;
  size_t plen, pcap, rlen, nameoff;
  struct walk_frame* f;
  int nf, fcap, next, err;
  struct dirlist kids; /* walk_kids(): the current directory's entries, until the next walk_read() */
  arena_pos kpos;      /* la before kids was read */
  struct stat st;
  dev_t rootdev;
  unsigned hasst : 1;
};

/* start a walk at root; 0, or -1 with errno ENOMEM. A root that cannot be
 * examined is reported by the first walk_read() (WALK_ERR), not here.
 *
 *   struct walk*  w      caller's handle
 *   const char*   root   file or directory
 *   unsigned      flags  WALK_PRE | WALK_POST | WALK_ERR | WALK_CYCLE | WALK_LOGICAL | ...
 * ----------------------------------------------------------------------- */
int walk_open(struct walk* w, const char* root, unsigned flags);

/* next visit, or NULL when done (errno 0) or out of memory (errno ENOMEM).
 * The entry is overwritten by the next call. */
struct walk_ent* walk_read(struct walk* w);

/* after a WALK_PRE: do not read this directory; its WALK_POST is not reported */
void walk_skip(struct walk* w);

/* the current entry stays (not removed, not changed): its parent directory
 * is reported with e->kept at WALK_POST, and so are the parent's parents */
void walk_keep(struct walk* w);

/* the entries of the current directory (a WALK_PRE or WALK_POST entry), as they are now: read when
 * asked, kept until the next walk_read(). At WALK_PRE the walk then goes through this very list
 * instead of reading the directory again. NULL with errno if it cannot be read (ENOTDIR for a
 * non-directory). Scan it with dirlist_foreach(); n == 0 is an empty directory.
 * ----------------------------------------------------------------------- */
const struct dirlist* walk_kids(struct walk* w);

/* stat of the current entry as the policy says (lstat, or stat when
 * followed); cached until the next walk_read(). NULL with errno on failure. */
const struct stat* walk_stat(struct walk* w);

void walk_close(struct walk* w);

#endif
