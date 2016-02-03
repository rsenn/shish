<<<<<<< HEAD
#ifndef WIN32
#include <unistd.h>
#endif
#include <stddef.h>
=======
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
>>>>>>> 6c7455723b47a4989fb5bb621be8f200a306f361
#include "uint32.h"
#include "open.h"

uint32        uint32_pool[UINT32_POOLSIZE];
unsigned long uint32_seeds = 0;

uint32 uint32_random(void)
{
  int i;
  uint32 r = 0;
  
  /* seed if not seeded */
  if(uint32_seeds == 0)
    uint32_seed(0, 0);

  for(i = 0; i < sizeof(uint32_pool) / sizeof(uint32); i++)
  {
    r += uint32_prng(uint32_pool[i], r);
    uint32_pool[i] = r;
  }

  if(uint32_seeds)
    uint32_seeds--;
  
  return r;
}
