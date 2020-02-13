/* this header file comes from libowfat, http://www.fefe.de / libowfat/ */
#ifndef UINT16_H
#define UINT16_H

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
#elif defined(__UINT16_TYPE__) && defined(__INT16_TYPE__)
typedef __UINT16_TYPE__ uint16;
typedef __INT16_TYPE__ int16;

#elif defined(___int16_t_defined) || defined(__BIT_TYPES_DEFINED__)
typedef u_int16_t uint16;
typedef int16_t int16;

#elif defined(_MSC_VER)
#include <windows.h>
typedef UINT16 uint16;
typedef INT16 int16;

#elif defined(__MINGW32__) || defined(__MINGW64__)
typedef uint16_t uint16;
typedef int16_t int16;

#else
typedef uint16_t uint16;
typedef int16_t int16;
#endif

#if !defined(NO_UINT16_MACROS)

#if(defined(__i386__) || defined(_M_IX86) || defined(_X86_) || defined(__x86_64__) || defined(_M_AMD64) ||             \
    defined(__LITTLE_ENDIAN__) || (BYTE_ORDER == _LITTLE_ENDIAN) || defined(_AMD64_) || defined(I_X86_))

inline static void
uint16_pack(char* out, uint16 in) {
  *(uint16*)out = in;
}

inline static void
uint16_unpack(const char* in, uint16* out) {
  *out = *(uint16*)in;
}

inline static uint16
uint16_get(const void* ptr) {
  const char* in = ptr;
  return *(uint16*)in;
}

inline static uint16
uint16_read(const char* in) {
  return *(uint16*)in;
}

#else

inline static uint16
uint16_get(const void* ptr) {
  const unsigned char* in = ptr;
  return ((uint16)in[0] << 8) | (in[1]);
}

inline static uint16
uint16_read(const char* in) {
  return ((uint16)in[0] << 8) | (in[1]);
}

void uint16_pack(char* out, uint16 in);
void uint16_unpack(const char* in, uint16* out);
#endif

#else
void uint16_pack(char* out, uint16 in);
void uint16_unpack(const char* in, uint16* out);
uint16 uint16_read(const char* in);
uint16 uint16_get(const void* ptr);
#endif

void uint16_pack_big(char* out, uint16 in);
void uint16_unpack_big(const char* in, uint16* out);
uint16 uint16_read_big(const char*);

#ifdef NO_UINT16_MACROS
uint16 uint16_read(const char*);
#endif

#ifdef __cplusplus
}
#endif

#endif
