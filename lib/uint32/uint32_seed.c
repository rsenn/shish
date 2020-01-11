#include "../open.h"
#include "../uint32.h"
#include <unistd.h>

extern uint32 uint32_pool[UINT32_POOLSIZE];

/* feed data to the prng */
int
uint32_seed(const void* p, unsigned long n) {
  int fd = -1;

  if(n == 0) {
    int i;

    fd = open_read("/dev/urandom");
    i = read(fd, uint32_pool, sizeof(uint32_pool));

    if(i > 0)
      uint32_seeds += i;

    close(fd);
  } else {
    const char* b = (const char*)p;
    char* x = (char*)uint32_pool;

    while(n) {
      x[n % sizeof(uint32_pool)] ^= *b;
      n--;
      b++;
      uint32_seeds++;
    }
  }

  return fd;
}
