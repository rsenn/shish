#ifndef UINT32_H
#define UINT32_H

#ifndef _WIN32
#include <inttypes.h>

typedef uint32_t uint32;
typedef int32_t int32;
#else
typedef unsigned __int32 uint32;
typedef signed __int32 int32;
#endif


/* bit rotating macros */
#define uint32_ror(v,c)      (((uint32)(v) >> (c)) | ((uint32)(v) << (32 - (c))))
#define uint32_rol(v,c)      (((uint32)(v) << (c)) | ((uint32)(v) >> (32 - (c))))
#define uint32_ror_safe(v,c) (((uint32)(v) >> uint32rc(c)) | ((uint32)(v) << (32 - uint32rc(c))))
#define uint32_rol_safe(v,c) (((uint32)(v) << uint32rc(c)) | ((uint32)(v) >> (32 - uint32rc(c))))

#if defined(__i386__) && !defined(NO_UINT32_MACROS)
#define uint32_pack(out,in) (*(uint32*)(out)=(in))
#define uint32_unpack(in,out) (*(out)=*(uint32*)(in))
#define uint32_read(in) (*(uint32*)(in))
void uint32_pack_big(char *out,uint32 in);
void uint32_unpack_big(const char *in,uint32* out);
uint32 uint32_read_big(const char *in);
#else

void uint32_pack(char *out,uint32 in);
void uint32_pack_big(char *out,uint32 in);
void uint32_unpack(const char *in,uint32* out);
void uint32_unpack_big(const char *in,uint32* out);
uint32 uint32_read(const char *in);
uint32 uint32_read_big(const char *in);

#endif

#define UINT32_POOLSIZE 16

extern uint32        uint32_pool[UINT32_POOLSIZE];
extern unsigned long uint32_seeds;

int uint32_seed(const void *p, unsigned long n);
uint32 uint32_prng(uint32 value, uint32 feedback);
uint32 uint32_random(void);
  

#endif
