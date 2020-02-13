/* this header file comes from libowfat, http://www.fefe.de / libowfat/ */
#ifndef UINT32_H
#define UINT32_H

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

#if defined(__BORLANDC__)
#elif defined(__UINT32_TYPE__) && defined(__INT32_TYPE__)
typedef __UINT32_TYPE__ uint32;
typedef __INT32_TYPE__ int32;

#elif defined(___int32_t_defined) || defined(__BIT_TYPES_DEFINED__)
typedef u_int32_t uint32;
typedef int32_t int32;

#elif defined(_MSC_VER)
#include <windows.h>
typedef UINT32 uint32;
typedef INT32 int32;

#elif defined(__MINGW32__) || defined(__MINGW64__)
typedef uint32_t uint32;
typedef int32_t int32;

#else
typedef uint32_t uint32;

#if !(defined(_WINSOCK2API_) && defined(__LCC__))
typedef int32_t int32;
#endif
#endif

#if !defined(NO_UINT32_MACROS)

#if(defined(__i386__) || defined(_M_IX86) || defined(_X86_) || defined(__x86_64__) || defined(_M_AMD64) ||             \
    defined(__LITTLE_ENDIAN__) || (BYTE_ORDER == _LITTLE_ENDIAN) || defined(_AMD64_) || defined(I_X86_))
inline static void
uint32_pack(char* out, uint32 in) {
  *(uint32*)out = in;
}

inline static void
uint32_unpack(const char* in, uint32* out) {
  *out = *(uint32*)in;
}

inline static uint32
uint32_get(const void* ptr) {
  const char* in = ptr;
  return *(uint32*)in;
}

inline static uint32
uint32_read(const char* in) {
  return *(uint32*)in;
}

#else

inline static uint32
uint32_get(const void* ptr) {
  const char* in = ptr;
  return (in[0] << 24) | (in[1] << 16) | (in[2] << 8) | (in[3]);
}

inline static uint32
uint32_read(const char* in) {
  return (in[0] << 24) | (in[1] << 16) | (in[2] << 8) | (in[3]);
}

void uint32_pack(char* out, uint32 in);
void uint32_unpack(const char* in, uint32* out);
#endif
#endif

void uint32_pack_big(char* out, uint32 in);
void uint32_unpack_big(const char* in, uint32* out);
uint32 uint32_read_big(const char* in);

uint32 uint32_random(void);
int uint32_seed(const void*, unsigned long n);
uint32 uint32_prng(uint32, uint32 seed);

/* bit rotating macros */
#define uint32_ror(v, c) (((uint32)(v) >> (c)) | ((uint32)(v) << (32 - (c))))
#define uint32_rol(v, c) (((uint32)(v) << (c)) | ((uint32)(v) >> (32 - (c))))
#define uint32_ror_safe(v, c) (((uint32)(v) >> uint32rc(c)) | ((uint32)(v) << (32 - uint32rc(c))))
#define uint32_rol_safe(v, c) (((uint32)(v) << uint32rc(c)) | ((uint32)(v) >> (32 - uint32rc(c))))

#ifdef __cplusplus
}
#endif
#endif
