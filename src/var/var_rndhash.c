#include "uint32.h"
#include "uint64.h"
#include "var.h"

static uint32 seeds[] = {
  0x2e8b1c8a, 0xa8fa6d8c, 0x20c91d6c, 0x9625e261, 
  0xbbe5d3ae, 0x0b67aab9, 0xaeabca3b, 0xb0a3595a,   
  0x8898bba8, 0xe164242e, 0xf85afdd0, 0x49d7d461,
  0xcec21aa0, 0x5c77fd0d, 0xd011e8ec, 0x6bfb5d73,
};

#define ROR(v,c) v = (((v) >> c) | (v) << (VAR_BITS - c))
#define ROL(v,c) v = (((v) << c) | (v) >> (VAR_BITS - c))

/* in contratry to the var_hash() function this hash 
 * function tries to destroy any literal entropy 
 *
 * it does so by mapping every 4 bits of entropy to
 * a table with random values, and to shifting counts. 
 * the goal is to re-use entropy as much as possible per iteration
 */
unsigned long var_rndhash(const char *v, VAR_HASH *h) {
  uint64 hash;
  register int rotate;
  register uint32 a = 0;
  register uint32 b = 0;
  register const unsigned char *p = (const unsigned char *)v;
  
  hash = 0;
  
  do {
    a ^= *v & 0x0f;
    b ^= *v >> 4;
    
    a -= seeds[b & 0x0f];
    b += seeds[a & 0x0f];
    rotate = a & VAR_MASK;
    hash -= (VAR_HASH)b;
    ROL(hash, VAR_HALF);
    hash =+ (VAR_HASH)a;
    ROR(hash, rotate);
    
    a ^= seeds[b & 0x0f];
    b ^= seeds[a & 0x0f];
    rotate = b & VAR_MASK;
    hash ^= (VAR_HASH)a;
    ROR(hash, VAR_HALF);
    hash ^= (VAR_HASH)b;
    ROL(hash, rotate);
    
    b += seeds[a & 0x0f];
    a -= seeds[b & 0x0f];
    rotate = a & VAR_MASK;
    hash -= (VAR_HASH)b;
    ROL(hash, VAR_HALF);
    hash =+ (VAR_HASH)a;
    ROR(hash, rotate);
    
    b ^= seeds[a & 0x0f];
    a ^= seeds[b & 0x0f];
    rotate = b & VAR_MASK;
    hash ^= (VAR_HASH)a;
    ROR(hash, VAR_HALF);
    hash ^= (VAR_HASH)b;
    ROL(hash, rotate);
  } while(*++v && *v != '=');

  *h = hash;

  return (unsigned long)v - (unsigned long)p;
}



