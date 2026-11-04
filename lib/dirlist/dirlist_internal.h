#ifndef DIRLIST_INTERNAL_H
#define DIRLIST_INTERNAL_H

#include "../dirlist.h"

/* growing blob while it is being read: s is the arena allocation of cap bytes */
struct dirlist_buf {
  struct dirlist d;
  size_t cap;
};

/* append one record, 0 on success, -1 on ENOMEM; name has nlen bytes */
int dirlist_add(arena* a, struct dirlist_buf* b, int type, const char* name, size_t nlen);

/* 1 if name is left out of the list under flags (DOTS, NOHIDDEN) */
int dirlist_skip(const char* name, unsigned flags);

#endif
