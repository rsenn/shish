/* this header file comes from libowfat, http://www.fefe.de/libowfat/ */
#ifndef UINT32_H
#define UINT32_H

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t uint32;
typedef int32_t int32;

#if (defined(__i386__) || defined(__x86_64__)) && !defined(NO_UINT32_MACROS)

static inline void uint32_pack(char* out, uint32 in) {
  *(uint32*)out = in;
}

static inline void uint32_unpack(const char *in, uint32* out) {
  *out = *(uint32*)in;
}

static inline uint32 uint32_read(const char* in) {
  return *(uint32*)in;
}

void uint32_pack_big(char *out, uint32 in);
void uint32_unpack_big(const char *in, uint32* out);
uint32 uint32_read_big(const char *in);
#else

void uint32_pack(char *out, uint32 in);
void uint32_pack_big(char *out, uint32 in);
void uint32_unpack(const char *in, uint32* out);
void uint32_unpack_big(const char *in, uint32* out);
uint32 uint32_read(const char *in);
uint32 uint32_read_big(const char *in);

#endif

#define uint32_ror(v,c)      (((uint32)(v) >> (c)) | ((uint32)(v) << (32 - (c))))
#define uint32_rol(v,c)      (((uint32)(v) << (c)) | ((uint32)(v) >> (32 - (c))))
#define uint32_ror_safe(v,c) (((uint32)(v) >> uint32rc(c)) | ((uint32)(v) << (32 - uint32rc(c))))
#define uint32_rol_safe(v,c) (((uint32)(v) << uint32rc(c)) | ((uint32)(v) >> (32 - uint32rc(c))))

#define UINT32_POOLSIZE 16

extern uint32        uint32_pool[UINT32_POOLSIZE];
extern unsigned long uint32_seeds;

int uint32_seed(const void *p, unsigned long n);
uint32 uint32_prng(uint32 value, uint32 feedback);
uint32 uint32_random(void);

#ifdef __cplusplus
}
#endif

#endif
