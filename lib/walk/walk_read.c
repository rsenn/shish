#include "walk_internal.h"
#include "../byte.h"
#include "../str.h"
#include <errno.h>

static int
type_of(mode_t m) {
  return S_ISDIR(m) ? DL_DIR : S_ISREG(m) ? DL_REG : S_ISLNK(m) ? DL_LNK : DL_OTHER;
}

/* path = the directory's path (dlen bytes) + '/' + name; 0, or -1 on ENOMEM */
static int
setpath(struct walk* w, size_t dlen, const char* name) {
  size_t n = str_len(name), need = dlen + n + 2;

  if(need > w->pcap) {
    size_t cap = w->pcap * 2 > need ? w->pcap * 2 : need;
    char* p;

    if(!(p = walk_grow(&w->pa, w->path, w->pcap, cap)))
      return -1;
    w->path = p;
    w->pcap = cap;
  }

  if(dlen && w->path[dlen - 1] != '/')
    w->path[dlen++] = '/';

  w->nameoff = dlen;
  byte_copy(w->path + dlen, n, name);
  w->plen = dlen + n;
  w->path[w->plen] = '\0';
  return 0;
}

/* a directory is entered only through here, so a symlink is followed only
 * when the caller asked: unless WALK_LOGICAL, d_type says DL_LNK and the
 * entry is a file, never a directory */
static void
classify(struct walk* w, int t) {
  w->hasst = 0;
  w->e.islink = t == DL_LNK;

  if(t == DL_UNKNOWN) {
    if(walk_lstat(w->path, &w->st) == 0) {
      w->hasst = 1;
      t = type_of(w->st.st_mode);
      w->e.islink = t == DL_LNK;
    } else {
      t = DL_OTHER;
    }
  }

  if(t == DL_LNK && (w->flags & WALK_LOGICAL)) {
    if(stat(w->path, &w->st) == 0) {
      w->hasst = 1;
      t = type_of(w->st.st_mode);
    } else if(walk_lstat(w->path, &w->st) == 0) {
      w->hasst = 1;
    }
  }

  w->e.type = t;
}

/* the directory about to be entered is one of the directories being listed */
static int
cycle(struct walk* w) {
#if WINDOWS_NATIVE
  (void)w;
  return 0;
#else
  const struct stat* st = walk_stat(w);
  int i;

  if(st)
    for(i = 0; i < w->nf; i++)
      if(w->f[i].dev == st->st_dev && w->f[i].ino == st->st_ino)
        return 1;
  return 0;
#endif
}

/* the phase of the entry just classified */
static int
visit(struct walk* w) {
  if(w->e.type != DL_DIR)
    return WALK_FILE;
  if((w->flags & WALK_LOGICAL) && cycle(w))
    return WALK_CYCLE;
  w->next = WN_DESCEND;
  return WALK_PRE;
}

static int
root(struct walk* w) {
  int follow = (w->flags & (WALK_LOGICAL | WALK_COMFOLLOW)) != 0, r;
  size_t n = w->plen;
  char c;

  w->nameoff = 0;
  w->e.depth = 0;
  w->hasst = 0;

  /* "link/" makes lstat() follow the link: without a follow flag look at "link" */
  while(!follow && n > 1 && w->path[n - 1] == '/')
    n--;

  c = w->path[n];
  w->path[n] = '\0';
  r = follow ? stat(w->path, &w->st) : walk_lstat(w->path, &w->st);
  w->path[n] = c;

  if(r == -1) {
    w->err = errno;
    return WALK_ERR;
  }

  w->hasst = 1;
  w->rootdev = w->st.st_dev;
  w->e.type = type_of(w->st.st_mode);
  w->e.islink = 0;

  if(follow) {
    struct stat ls;

    w->e.islink = walk_lstat(w->path, &ls) == 0 && S_ISLNK(ls.st_mode);
  }

  return visit(w);
}

/* enter the directory at w->path: push its frame and read it. Returns
 * WALK_ERR (w->err set, nothing pushed) or 0. */
static int
descend(struct walk* w) {
  const struct stat* st;
  struct walk_frame* f;
  struct dirlist d = {0, 0, 0};
  int stop;

  if(w->nf == w->fcap) {
    struct walk_frame* nf;

    if(!(nf = walk_grow(&w->fa, w->f, w->fcap * sizeof(*w->f), w->fcap * 2 * sizeof(*w->f)))) {
      w->err = ENOMEM;
      return WALK_ERR;
    }
    w->f = nf;
    w->fcap *= 2;
  }

  /* looked at again right before opening: a directory that has been
     replaced by a symlink since it was listed is not entered */
  if(!(st = walk_stat(w))) {
    w->err = errno;
    return WALK_ERR;
  }

  if(!S_ISDIR(st->st_mode)) {
    w->err = ENOTDIR;
    return WALK_ERR;
  }

  f = &w->f[w->nf];
  f->pos = arena_tell(&w->la);
  f->plen = w->plen;
  f->nameoff = w->nameoff;
  f->dev = st->st_dev;
  f->ino = st->st_ino;
  f->islink = w->e.islink;
  f->kept = 0;
  f->cur = f->end = NULL;

  stop = (w->maxdepth >= 0 && w->nf >= w->maxdepth) || ((w->flags & WALK_XDEV) && st->st_dev != w->rootdev);

  if(stop && w->kids.s) {
    /* asked for, but not to be walked */
    arena_rewind(&w->la, w->kpos);
    f->pos = w->kpos;
    w->kids.s = NULL;
  }

  if(!stop) {
    if(w->kids.s) {
      /* walk_kids() read it already: that list is the level */
      d = w->kids;
      f->pos = w->kpos;
      w->kids.s = NULL;
    } else {
      d = dirlist_read(&w->la, w->path, 0);
    }

    if(!d.s) {
      w->err = errno;
      return WALK_ERR;
    }

    f->cur = d.s;
    f->end = d.s + d.len;
  }

  w->nf++;
  return 0;
}

/* the next entry of the directory f, or -1 on ENOMEM */
static int
child(struct walk* w, struct walk_frame* f) {
  const char* rec = f->cur;

  f->cur = dirlist_next(f->cur);

  if(setpath(w, f->plen, dirlist_name(rec)))
    return -1;

  w->e.depth = w->nf;
  classify(w, dirlist_type(rec));
  return visit(w);
}

/* the directory is listed completely */
static int
pop(struct walk* w) {
  struct walk_frame* f = &w->f[--w->nf];

  arena_rewind(&w->la, f->pos);
  w->plen = f->plen;
  w->path[w->plen] = '\0';
  w->nameoff = f->nameoff;
  w->hasst = 0;
  w->e.depth = w->nf;
  w->e.type = DL_DIR;
  w->e.islink = f->islink;
  w->e.kept = f->kept;

  if(f->kept && w->nf)
    f[-1].kept = 1;
  return WALK_POST;
}

struct walk_ent*
walk_read(struct walk* w) {
  struct walk_ent* e = &w->e;
  int ph;

  for(;;) {
    e->kept = 0;

    /* a list nobody descended through (skipped, or a POST entry's) is dropped */
    if(w->kids.s && w->next != WN_DESCEND) {
      arena_rewind(&w->la, w->kpos);
      w->kids.s = NULL;
    }

    if(w->next == WN_DONE)
      return NULL;

    if(w->next == WN_ROOT) {
      w->next = WN_LIST;
      ph = root(w);
    } else if(w->next == WN_DESCEND) {
      w->next = WN_LIST;
      ph = descend(w);
    } else if(!w->nf) {
      w->next = WN_DONE;
      errno = 0;
      return NULL;
    } else if(w->f[w->nf - 1].cur < w->f[w->nf - 1].end) {
      if((ph = child(w, &w->f[w->nf - 1])) < 0) {
        w->next = WN_DONE;
        errno = ENOMEM;
        return NULL;
      }
    } else {
      ph = pop(w);
    }

    if(ph == WALK_FILE || (ph & (int)w->flags)) {
      e->phase = ph;
      e->err = ph == WALK_ERR ? w->err : 0;
      e->path = w->path;
      e->len = w->plen;
      e->name = w->path + w->nameoff;
      e->rel = w->path + (w->plen > w->rlen ? w->rlen : w->plen);
      return e;
    }
  }
}
