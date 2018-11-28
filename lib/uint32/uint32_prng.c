#include "uint32.h"

static uint32 seeds[8] = {
    0xe25e40f8,
    0x2839fba7,
    0x4e75896d,
    0x69291d9b,
    0xdfaf0ba6,
    0x9a6e9617,
    0x432b04e5,
    0x434846dc,
};

/* prng */
uint32
uint32_prng(uint32 value, uint32 seed) {
  register int i = 27;
  while(i >= 0) {
    /* whatever, try to re-use entropy as many as possible with
       as less as possible instructions */
    seed ^= uint32_ror(seeds[((value + seed) >> (i + 2)) & 7], seed & 0x1f);
    seed += uint32_rol(seeds[((value - seed) >> (i + 1)) & 7], seed & 0x1f);
    seed -= uint32_ror(seeds[((value ^ seed) >> (i)) & 7], seed & 0x1f);
    i--;
  }

  return seed;
}
