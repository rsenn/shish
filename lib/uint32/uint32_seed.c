<<<<<<< HEAD
#ifndef WIN32
=======
#ifdef _WIN32
#include <io.h>
#else
>>>>>>> 6c7455723b47a4989fb5bb621be8f200a306f361
#include <unistd.h>
#endif
#include "uint32.h"
#include "open.h"

/* feed data to the prng */
int uint32_seed(const void *p, unsigned long n)
{
  int fd = -1;
  
  if(n == 0)
  {
    int i;
    
    fd = open_read("/dev/urandom");
    i = read(fd, uint32_pool, sizeof(uint32_pool));
    
    if(i > 0)
      uint32_seeds += i;
    
    close(fd);
  }
  else
  {
    const char *b = (const char *)p;
    char *x = (char *)uint32_pool;
    
    while(n)
    {
      x[n % sizeof(uint32_pool)] ^= *b;
      n--;
      b++;
      uint32_seeds++;
    }
    
  }
  
  return fd;
}
