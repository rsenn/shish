#ifndef UNIX_H
#define UNIX_H 1

#include "typedefs.h"
#include "windoze.h"
#include "getopt.h"

#if !WINDOWS_NATIVE
#include <unistd.h>
#endif

extern const short __spm[13];

char is_junction(const char* LinkPath);
int is_symlink(const char* LinkPath);

#if WINDOWS_NATIVE
ssize_t readlink(const char* LinkPath, char* buf, size_t maxlen);
size_t getpagesize();
#endif

#endif /* defined(UNIX_H) */
