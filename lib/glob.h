#ifndef GLOB_H
#define GLOB_H

#include "typedefs.h"

#define GLOB_NOSPACE 1
#define GLOB_ABORTED 2
#define GLOB_NOMATCH 3
#define GLOB_NOSYS 4
#define GLOB_ABEND GLOB_ABORTED

#define GLOB_ERR 0x00000001
#define GLOB_MARK 0x00000002
#define GLOB_NOSORT 0x00000004
#define GLOB_DOOFFS 0x00000008
#define GLOB_NOCHECK 0x00000010
#define GLOB_APPEND 0x00000020
#define GLOB_NOESCAPE 0x00000040

#define GLOB_PERIOD 0x00000080
#define GLOB_MAGCHAR 0x00000100
#define GLOB_BRACE 0x00000400
#define GLOB_NOMAGIC 0x00000800
#define GLOB_ONLYDIR 0x00002000
#define GLOB_TILDE_CHECK 0x00004000
#define GLOB_ONLYFILE 0x00008000
#define GLOB_NODOTSDIRS 0x00010000
#define GLOB_LIMIT 0x00020000

typedef struct {
  size_t gl_pathc;
  char** gl_pathv;
  size_t gl_offs;
  int gl_flags;
  size_t gl_matchc;
} glob_t;

int glob(const char* pattern,
         int flags,
         int (*errfunc)(const char*, int),
         glob_t* pglob);
void globfree(glob_t* pglob);

#endif
