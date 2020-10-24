/* this header file comes from libowfat, http://www.fefe.de / libowfat/ */
#ifndef UINT64_H
#define UINT64_H

#if defined(__BORLANDC__)
#include <systypes.h>
#elif defined(__LCC__)
#include <stdint.h>
#elif !defined(_MSC_VER) && !defined(__MSYS__) && !defined(__CYGWIN__)
#include <inttypes.h>
#include <stdint.h>
#else
#define __MS_types__
#include <sys/types.h>
#endif

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__UINT64_TYPE__) && defined(__INT64_TYPE__)
typedef __UINT64_TYPE__ uint64;
typedef __INT64_TYPE__ int64;

#elif defined(___int64_t_defined) || defined(__BIT_TYPES_DEFINED__)
typedef u_int64_t uint64;
typedef int64_t int64;

#elif defined(_MSC_VER) || defined(__BORLANDC__)
#include <windows.h>
typedef UINT64 uint64;
typedef INT64 int64;

#elif defined(__MINGW32__) || defined(__MINGW64__)
typedef uint64_t uint64;
typedef int64_t int64;

#else
typedef uint64_t uint64;
typedef int64_t int64;
#endif

#if !defined(NO_UINT64_MACROS)

#if(defined(__i386__) || defined(_M_IX86) || defined(_X86_) || defined(__x86_64__) || defined(_M_AMD64) ||             \
    defined(__LITTLE_ENDIAN__) || (BYTE_ORDER == _LITTLE_ENDIAN) || defined(_AMD64_) || defined(I_X86_))

#define uint64_pack(out, in) (*(uint64*)(out) = (in))
#define uint64_unpack(in, out) (*(out) = *(uint64*)(in))
#define uint64_read(in) (*(uint64*)(in))

inline static uint64
uint64_get(const void* ptr) {
  const char* in = ptr;
  return *(uint64*)in;
}

#else
void uint64_pack(char* out, uint64 in);
void uint64_unpack(const char* in, uint64* out);
uint64 uint64_read(const char* in);
#endif
#endif
void uint64_pack_big(char* out, uint64 in);
void uint64_unpack_big(const char* in, uint64* out);
uint64 uint64_read_big(const char* in);

#if defined(_WIN32) && defined(_MSC_VER)
// for older MSVC
#ifndef PRId64
#define PRId64 "I64d"
#endif
#ifndef PRIu64
#define PRIu64 "I64u"
#endif
#ifndef PRIx64
#define PRIx64 "I64x"
#endif
#endif /* _WIN32 && _MSC_VER */

#if defined(_WIN32) && defined(_MSC_VER)
// for older MSVC: "unsigned __int64 -> double" conversion not implemented (why?-)
__inline double
uint64_to_double(uint64 ull) {
  return ((int64)ull >= 0 ? (double)(int64)ull
                          : ((double)(int64)(ull - 9223372036854775808UI64)) + 9223372036854775808.0);
}
#else
#define uint64_to_double(ull) ((double)(ull))
#endif /* _WIN32 && _MSC_VER && TSCI2_OS_WIN32 */

#ifdef __cplusplus
}
#endif

#endif
