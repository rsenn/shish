#ifndef WINDOZE_H
#define WINDOZE_H 1

#if (defined(WIN32) || defined(__MINGW32__)) && !defined(_WIN32)
#define _WIN32 1
#endif

#if (defined(WIN64) || defined(__MINGW64__)) && !defined(_WIN64)
#define _WIN64 1
#endif

#if defined(_WIN32) || defined(_WIN64) || defined(__MINGW32__) || defined(__MINGW64__) || \
    defined(MSVC) || defined(__CYGWIN__) || defined(__MSYS__)
#if !(defined(__MSYS__) || defined(__CYGWIN__))
#define WINDOWS_NATIVE 1
#endif
#ifndef WINDOWS
#define WINDOWS 1
#endif
#endif

#if defined(__MINGW32__) || defined(__MINGW64__)
#define MINGW 1
#endif


#define _FILE_OFFSET_BITS 64

#if !WINDOWS_NATIVE
#define WINDOWS_NATIVE 0

#if defined(__MSYS__)
#define MSYS 1
#endif

#if defined(__CYGWIN__)
#define CYGWIN 1
#endif
#endif

#if WINDOWS_NATIVE

#ifdef _MSC_VER
#define _CRT_INTERNAL_NONSTDC_NAMES 1
#endif

#include <io.h>
#include <direct.h>

#if !defined(__LCC__) && !defined(__MINGW32__)
#define read _read
#define write _write
#define open _open
#define close _close
#endif

#ifndef MINGW
#ifndef HAVE_DEV_T 
typedef int dev_t;
#endif
#ifndef HAVE_PID_T
typedef int pid_t;
#endif
#endif

/* int _mkdir(const char *); */

#define mkdir(path, mode) _mkdir(path)

/* int _pipe(int [2], unsigned int, int); */

#define pipe(fds) _pipe(fds, 8192, _O_BINARY)

#endif

#endif
