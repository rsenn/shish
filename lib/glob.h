#ifndef GLOB_H
#define GLOB_H

#include "typedefs.h"

#define GLOB_NOSPACE 1
#define GLOB_ABORTED 2
#define GLOB_NOMATCH 3
#define GLOB_NOSYS 4
#define GLOB_ABEND GLOB_ABORTED

#define GLOB_ERR 0x01
#define GLOB_MARK 0x02
#define GLOB_NOSORT 0x04
#define GLOB_DOOFFS 0x08
#define GLOB_NOCHECK 0x10
#define GLOB_APPEND 0x20
#define GLOB_NOESCAPE 0x40

#define GLOB_PERIOD 0x80
#define GLOB_MAGCHAR 0x0100
#define GLOB_BRACE 0x0400
#define GLOB_NOMAGIC 0x0800
#define GLOB_TILDE 0x1000
#define GLOB_ONLYDIR 0x2000
#define GLOB_TILDE_CHECK 0x4000
#define GLOB_ONLYFILE 0x8000
#define GLOB_NODOTSDIRS 0x10000
#define GLOB_LIMIT 0x20000

typedef struct {
  size_t gl_pathc;
  char** gl_pathv;
  size_t gl_offs;
  int gl_flags;
  size_t gl_matchc;
} glob_t;

int glob(const char* pattern, int flags, int (*errfunc)(const char*, int), glob_t* pglob);
void globfree(glob_t* pglob);

#endif
