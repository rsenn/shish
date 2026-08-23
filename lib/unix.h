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
#include <sys/types.h>
#include <sys/stat.h>

ssize_t readlink(const char* LinkPath, char* buf, size_t maxlen);
int link(const char* oldpath, const char* newpath);
int symlink(const char* oldpath, const char* newpath);
size_t getpagesize();
pid_t getppid(void);
int kill(pid_t pid, int sig);
int killpg(pid_t pgrp, int sig);
int fork(void);
#include <process.h> /* declares execve() (deprecated but real) */

/* mingw's <sys/stat.h> has no symlink bit at all -- is_symlink()
 * already detects one via its reparse-point tag, so give it a real
 * S_IFLNK/S_ISLNK/lstat() instead of leaving them undeclared. */
#ifndef S_IFLNK
#define S_IFLNK 0xA000
#endif
#ifndef S_ISLNK
#define S_ISLNK(m) (((m)&S_IFMT) == S_IFLNK)
#endif
int lstat(const char* path, struct stat* buf);
#endif

/* platforms without a real uname(2)/<sys/utsname.h>: Windows, and
 * Emscripten/WASI/bare-wasm targets, which have no host to introspect. */
#if WINDOWS_NATIVE || defined(__EMSCRIPTEN__) || defined(__wasm__) || defined(__wasi__)
struct utsname {
  char sysname[65];
  char nodename[65];
  char release[65];
  char version[65];
  char machine[65];
};

int uname(struct utsname* buf);
#endif

#endif /* defined(UNIX_H) */
