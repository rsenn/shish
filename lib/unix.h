#ifndef UNIX_H
#define UNIX_H 1

#include "typedefs.h"
#include "windoze.h"
#include "getopt.h"

#if !WINDOWS_NATIVE
#include <unistd.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern const short __spm[13];

char is_junction(const char* LinkPath);
int is_symlink(const char* LinkPath);
ssize_t readlink(const char* LinkPath, char* buf, size_t maxlen);

#if WINDOWS_NATIVE
size_t getpagesize();
#endif

#ifdef __cplusplus
}
#endif
#endif /* defined(UNIX_H) */
